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
 **   Copyright (C) 2020 - 2021. GekkoFyre.
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
#include <QtGlobal>
#include <QMessageBox>
#include <QImageReader>
#include <QMimeDatabase>

#if defined(_WIN32) || defined(__MINGW64__) || defined(__CYGWIN__)
#include <initguid.h>
#include <netlistmgr.h>
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

GkSystem::GkSystem(QPointer<GekkoFyre::StringFuncs> stringFuncs, QObject *parent) : QObject(parent)
{
    setParent(parent);
    gkStringFuncs = std::move(stringFuncs);

    return;
}

GkSystem::~GkSystem()
{
    return;
}

/**
 * @brief GkSystem::getImgFormat obtain the MIME data of the given QByteArray of raw data (i.e. image format such as
 * either 'PNG' or 'JPEG') through analysis of said raw data, and return it as a QString.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param data The QByteArray of raw data for the analysis to be made against.
 * @param suffix Whether to return just the suffix (i.e. 'jpeg') instead of just the actual, full MIME format (i.e.
 * image/jpeg).
 * @return The detected MIME data/format from the given QByteArray of raw data, as a QString for easier use of application.
 */
QString GkSystem::getImgFormat(const QByteArray &data, const bool &suffix)
{
    try {
        std::unique_ptr<QMimeDatabase> mime_db = std::make_unique<QMimeDatabase>();
        const auto mime_type = mime_db->mimeTypeForData(data);

        //
        // Ensure that the image is supported by the Qt Project set of libraries!
        const QString dataStr = QString::fromUtf8(data);
        const auto det_type = isImgFormatSupported(mime_type, dataStr, suffix);
        return det_type;
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));
    }

    return "application/octet-stream";
}

/**
 * @brief GkSystem::isImgFormatSupported whether the given image format (i.e. MIME data) is supported by the Qt Project
 * set of libraries or not.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param mime_type The MIME data to make the comparison against within the Qt Project MIME database for image formats.
 * @param str The QString to make the MIME data comparison against with `mime_type`.
 * @param suffix Whether to return just the suffix (i.e. 'jpeg') instead of just the actual, full MIME format (i.e.
 * image/jpeg).
 * @return The detected MIME data/format from the given QByteArray of raw data, as a QString for easier use of application.
 */
QString GkSystem::isImgFormatSupported(const QMimeType &mime_type, const QString &str, const bool &suffix)
{
    try {
        //
        // Ensure that the image is supported by the Qt Project set of libraries!
        for (const auto &det_suffix: mime_type.suffixes()) {
            for (const auto &supported_img: QImageReader::supportedImageFormats()) {
                //
                // Now compare against the list of supported image formats for within the Qt Project set
                // of libraries!
                if (suffix) { // Only make use of a suffix!
                    if (str.contains(det_suffix, Qt::CaseInsensitive)) {
                        return det_suffix;
                    }
                } else {
                    if (det_suffix == supported_img) {
                        return det_suffix;
                    }
                }
            }
        }
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));
    }

    return "application/octet-stream";
}

/**
 * @brief GkSystem::isInternetAvailable checks the connectivity of the user's own, local computer/machine and determines
 * if an Internet connection is readily available. If so, returns true but otherwise, it will return false. This works
 * for both Microsoft Windows and Linux operating systems.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return A true boolean value will be returned if a readily available Internet connection is usable on the user's own,
 * local machine but otherwise, a false boolean value will be returned.
 */
bool GkSystem::isInternetAvailable() {
    #if defined(_WIN32) || defined(__MINGW64__) || defined(__CYGWIN__)
    std::unique_ptr<INetworkListManager> inlm;
    HRESULT hr = CoCreateInstance(CLSID_NetworkListManager, nullptr, CLSCTX_ALL, IID_INetworkListManager, (LPVOID *)&inlm);
    if (SUCCEEDED(hr)) {
        NLM_CONNECTIVITY con;
        hr = inlm->GetConnectivity(&con);
        if (hr == S_OK) {
            if (con & NLM_CONNECTIVITY_IPV4_INTERNET || con & NLM_CONNECTIVITY_IPV6_INTERNET) {
                return true;
            } else {
                return false;
            }
        }

        return true;
    }
    #elif __linux__
    //
    // TODO: Insert a check to see that `/sbin/route` is at all installed on the end-user's computer/machine, instead of blindly
    // running the command and hoping for the best!
    //
    FILE *output;
    if (!(output = popen("/sbin/route -n | grep -c '^0\\.0\\.0\\.0'", "r"))) {
        return true;
    }

    unsigned int i;
    fscanf(output, "%u", &i);
    pclose(output);

    if (i == 0) {
        return false;
    } else {
        return true;
    }
    #endif

    return false;
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
    sysctl(nm, 2, &count, &len, nullptr, 0);

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

#if defined(_WIN32) || defined(__MINGW64__) || defined(__CYGWIN__)
/**
 * @brief GkSystem::WindowsFirewallInitialize
 * @author Microsoft Corporation <https://docs.microsoft.com/en-us/previous-versions//aa364726(v=vs.85)?redirectedfrom=MSDN>,
 * Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @note Exercising the Firewall using C++ <https://docs.microsoft.com/en-us/previous-versions//aa364726(v=vs.85)?redirectedfrom=MSDN>.
 */
HRESULT GkSystem::WindowsFirewallInitialize(INetFwProfile **fwProfile)
{
    HRESULT hr = NOERROR;
    INetFwMgr* fwMgr = nullptr;
    INetFwPolicy* fwPolicy = nullptr;

    Q_ASSERT(fwProfile != nullptr);
    *fwProfile = nullptr;

    // Create an instance of the firewall settings manager.
    hr = CoCreateInstance(__uuidof(NetFwMgr), nullptr, CLSCTX_INPROC_SERVER, __uuidof(INetFwMgr), (void**)&fwMgr);
    if (FAILED(hr)) {
        auto errMsg = processHResult(hr);
        emit publishEventMsg(tr("CoCreateInstance failed: 0x%1lx").arg(gkStringFuncs->zeroPadding(errMsg, GK_MICROSOFT_HR_ERROR_MSG_ZERO_PADDING)), GkSeverity::Fatal, "", false, true, false, true);
        goto error;
    }

    // Retrieve the local firewall policy.
    hr = fwMgr->get_LocalPolicy(&fwPolicy);
    if (FAILED(hr)) {
        auto errMsg = processHResult(hr);
        emit publishEventMsg(tr("get_LocalPolicy failed: 0x%1lx").arg(gkStringFuncs->zeroPadding(errMsg, GK_MICROSOFT_HR_ERROR_MSG_ZERO_PADDING)), GkSeverity::Fatal, "", false, true, false, true);
        goto error;
    }

    // Retrieve the firewall profile currently in effect.
    hr = fwPolicy->get_CurrentProfile(fwProfile);
    if (FAILED(hr)) {
        auto errMsg = processHResult(hr);
        emit publishEventMsg(tr("get_CurrentProfile failed: 0x%1lx").arg(gkStringFuncs->zeroPadding(errMsg, GK_MICROSOFT_HR_ERROR_MSG_ZERO_PADDING)), GkSeverity::Fatal, "", false, true, false, true);
        goto error;
    }

    error:
    // Release the local firewall policy.
    if (fwPolicy != nullptr) {
        fwPolicy->Release();
    }

    // Release the firewall settings manager.
    if (fwMgr != nullptr) {
        fwMgr->Release();
    }

    return hr;
}
#endif

#if defined(_WIN32) || defined(__MINGW64__) || defined(__CYGWIN__)
/**
 * @brief GkSystem::WindowsFirewallIsOn
 * @author Microsoft Corporation <https://docs.microsoft.com/en-us/previous-versions//aa364726(v=vs.85)?redirectedfrom=MSDN>,
 * Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param fwProfile
 * @param fwOn
 * @return
 * @note Exercising the Firewall using C++ <https://docs.microsoft.com/en-us/previous-versions//aa364726(v=vs.85)?redirectedfrom=MSDN>.
 */
HRESULT GkSystem::WindowsFirewallIsOn(INetFwProfile *fwProfile, WINBOOL *fwOn)
{
    HRESULT hr = NOERROR;
    VARIANT_BOOL fwEnabled;

    Q_ASSERT(fwProfile != nullptr);
    Q_ASSERT(fwOn != nullptr);

    *fwOn = FALSE;

    // Get the current state of the firewall.
    hr = fwProfile->get_FirewallEnabled(&fwEnabled);
    if (FAILED(hr)) {
        auto errMsg = processHResult(hr);
        emit publishEventMsg(tr("get_FirewallEnabled failed: 0x%1lx").arg(gkStringFuncs->zeroPadding(errMsg, GK_MICROSOFT_HR_ERROR_MSG_ZERO_PADDING)), GkSeverity::Fatal, "", false, true, false, true);
        goto error;
    }

    // Check to see if the firewall is on.
    if (fwEnabled != VARIANT_FALSE) {
        *fwOn = TRUE;
        emit publishEventMsg(tr("The firewall is on."), GkSeverity::Debug, "", false, true, false, false);
    } else {
        emit publishEventMsg(tr("The firewall is off."), GkSeverity::Debug, "", false, true, false, false);
    }

    error:
    return hr;
}
#endif

#if defined(_WIN32) || defined(__MINGW64__) || defined(__CYGWIN__)
/**
 * @brief GkSystem::processHResult
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param hr
 * @return
 */
QString GkSystem::processHResult(const HRESULT &hr)
{
    _com_error err(hr);
    LPCTSTR errMsg = err.ErrorMessage();
    QString errMsgStr = (LPSTR)errMsg;

    return errMsgStr;
}
#endif
