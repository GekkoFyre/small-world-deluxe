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
#include <QMessageBox>
#include <QtGlobal>
#include <comdef.h>

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
    HRESULT hr = S_OK;
    INetFwMgr* fwMgr = nullptr;
    INetFwPolicy* fwPolicy = nullptr;

    Q_ASSERT(fwProfile != nullptr);
    *fwProfile = nullptr;

    // Create an instance of the firewall settings manager.
    hr = CoCreateInstance(__uuidof(NetFwMgr), nullptr, CLSCTX_INPROC_SERVER, __uuidof(INetFwMgr), (void**)&fwMgr);
    if (FAILED(hr)) {
        _com_error err(hr);
        LPCTSTR errMsg = err.ErrorMessage();
        QString errMsgStr = (LPSTR)errMsg;
        emit publishEventMsg(tr("CoCreateInstance failed: 0x%1lx").arg(gkStringFuncs->zeroPadding(errMsgStr, GK_MICROSOFT_HR_ERROR_MSG_ZERO_PADDING)), GkSeverity::Fatal, "", false, true, false, true);
        goto error;
    }

    // Retrieve the local firewall policy.
    hr = fwMgr->get_LocalPolicy(&fwPolicy);
    if (FAILED(hr)) {
        _com_error err(hr);
        LPCTSTR errMsg = err.ErrorMessage();
        QString errMsgStr = (LPSTR)errMsg;
        emit publishEventMsg(tr("get_LocalPolicy failed: 0x%1lx").arg(gkStringFuncs->zeroPadding(errMsgStr, GK_MICROSOFT_HR_ERROR_MSG_ZERO_PADDING)), GkSeverity::Fatal, "", false, true, false, true);
        goto error;
    }

    // Retrieve the firewall profile currently in effect.
    hr = fwPolicy->get_CurrentProfile(fwProfile);
    if (FAILED(hr)) {
        _com_error err(hr);
        LPCTSTR errMsg = err.ErrorMessage();
        QString errMsgStr = (LPSTR)errMsg;
        emit publishEventMsg(tr("get_CurrentProfile failed: 0x%1lx").arg(gkStringFuncs->zeroPadding(errMsgStr, GK_MICROSOFT_HR_ERROR_MSG_ZERO_PADDING)), GkSeverity::Fatal, "", false, true, false, true);
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
    HRESULT hr = S_OK;
    VARIANT_BOOL fwEnabled;

    Q_ASSERT(fwProfile != nullptr);
    Q_ASSERT(fwOn != nullptr);

    *fwOn = FALSE;

    // Get the current state of the firewall.
    hr = fwProfile->get_FirewallEnabled(&fwEnabled);
    if (FAILED(hr)) {
        _com_error err(hr);
        LPCTSTR errMsg = err.ErrorMessage();
        QString errMsgStr = (LPSTR)errMsg;
        emit publishEventMsg(tr("get_FirewallEnabled failed: 0x%1lx").arg(gkStringFuncs->zeroPadding(errMsgStr, GK_MICROSOFT_HR_ERROR_MSG_ZERO_PADDING)), GkSeverity::Fatal, "", false, true, false, true);
        goto error;
    }

    // Check to see if the firewall is on.
    if (fwEnabled != VARIANT_FALSE) {
        *fwOn = TRUE;
        emit publishEventMsg(tr("The firewall is on."), GkSeverity::Debug, "", true, true, false, false);
    } else {
        emit publishEventMsg(tr("The firewall is off."), GkSeverity::Debug, "", true, true, false, false);
    }

    error:
    return hr;
}
#endif

#if defined(_WIN32) || defined(__MINGW64__) || defined(__CYGWIN__)
/**
 * @brief GkSystem::WindowsFirewallAppIsEnabled
 * @author Microsoft Corporation <https://docs.microsoft.com/en-us/previous-versions//aa364726(v=vs.85)?redirectedfrom=MSDN>,
 * Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param fwProfile
 * @param fwProcessImageFileName
 * @param fwAppEnabled
 * @return
 * @note Exercising the Firewall using C++ <https://docs.microsoft.com/en-us/previous-versions//aa364726(v=vs.85)?redirectedfrom=MSDN>.
 */
HRESULT GkSystem::WindowsFirewallAppIsEnabled(INetFwProfile *fwProfile, const wchar_t *fwProcessImageFileName, WINBOOL *fwAppEnabled)
{
    HRESULT hr = S_OK;
    BSTR fwBstrProcessImageFileName = nullptr;
    VARIANT_BOOL fwEnabled;
    INetFwAuthorizedApplication* fwApp = nullptr;
    INetFwAuthorizedApplications* fwApps = nullptr;

    Q_ASSERT(fwProfile != nullptr);
    Q_ASSERT(fwProcessImageFileName != nullptr);
    Q_ASSERT(fwAppEnabled != nullptr);

    *fwAppEnabled = FALSE;

    // Retrieve the authorized application collection.
    hr = fwProfile->get_AuthorizedApplications(&fwApps);
    if (FAILED(hr)) {
        _com_error err(hr);
        LPCTSTR errMsg = err.ErrorMessage();
        QString errMsgStr = (LPSTR)errMsg;
        emit publishEventMsg(tr("get_AuthorizedApplications failed: 0x%1lx").arg(gkStringFuncs->zeroPadding(errMsgStr, GK_MICROSOFT_HR_ERROR_MSG_ZERO_PADDING)), GkSeverity::Fatal, "", false, true, false, true);
        goto error;
    }

    // Allocate a BSTR for the process image file name.
    fwBstrProcessImageFileName = SysAllocString(fwProcessImageFileName);
    if (fwBstrProcessImageFileName == nullptr) {
        hr = E_OUTOFMEMORY;
        _com_error err(hr);
        LPCTSTR errMsg = err.ErrorMessage();
        QString errMsgStr = (LPSTR)errMsg;
        emit publishEventMsg(tr("SysAllocString failed: 0x%1lx").arg(gkStringFuncs->zeroPadding(errMsgStr, GK_MICROSOFT_HR_ERROR_MSG_ZERO_PADDING)), GkSeverity::Fatal, "", false, true, false, true);
        goto error;
    }

    // Attempt to retrieve the authorized application.
    hr = fwApps->Item(fwBstrProcessImageFileName, &fwApp);
    if (SUCCEEDED(hr)) {
        // Find out if the authorized application is enabled.
        hr = fwApp->get_Enabled(&fwEnabled);
        if (FAILED(hr)) {
            _com_error err(hr);
            LPCTSTR errMsg = err.ErrorMessage();
            QString errMsgStr = (LPSTR)errMsg;
            emit publishEventMsg(tr("get_Enabled failed: 0x%1lx").arg(gkStringFuncs->zeroPadding(errMsgStr, GK_MICROSOFT_HR_ERROR_MSG_ZERO_PADDING)), GkSeverity::Fatal, "", false, true, false, true);
            goto error;
        }

        if (fwEnabled != VARIANT_FALSE) {
            // The authorized application is enabled.
            *fwAppEnabled = TRUE;
            emit publishEventMsg(tr("Authorized application %1 is enabled in the firewall.").arg(QString::fromWCharArray(fwProcessImageFileName)), GkSeverity::Debug, "", true, true, false, false);
        } else {
            emit publishEventMsg(tr("Authorized application %1 is disabled in the firewall.").arg(QString::fromWCharArray(fwProcessImageFileName)), GkSeverity::Debug, "", true, true, false, false);
        }
    } else {
        // The authorized application was not in the collection.
        hr = S_OK;
        emit publishEventMsg(tr("Authorized application %1 is disabled in the firewall.").arg(QString::fromWCharArray(fwProcessImageFileName)), GkSeverity::Debug, "", true, true, false, false);
    }

    error:
    // Free the BSTR.
    SysFreeString(fwBstrProcessImageFileName);

    // Release the authorized application instance.
    if (fwApp != nullptr) {
        fwApp->Release();
    }

    // Release the authorized application collection.
    if (fwApps != nullptr) {
        fwApps->Release();
    }

    return hr;
}
#endif

#if defined(_WIN32) || defined(__MINGW64__) || defined(__CYGWIN__)
/**
 * @brief GkSystem::WindowsFirewallTurnOn
 * @author Microsoft Corporation <https://docs.microsoft.com/en-us/previous-versions//aa364726(v=vs.85)?redirectedfrom=MSDN>,
 * Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @note Exercising the Firewall using C++ <https://docs.microsoft.com/en-us/previous-versions//aa364726(v=vs.85)?redirectedfrom=MSDN>.
 */
HRESULT GkSystem::WindowsFirewallTurnOn(INetFwProfile *fwProfile)
{
    HRESULT hr = S_OK;
    BOOL fwOn;

    Q_ASSERT(fwProfile != nullptr);

    // Check to see if the firewall is off.
    hr = WindowsFirewallIsOn(fwProfile, &fwOn);
    if (FAILED(hr)) {
        _com_error err(hr);
        LPCTSTR errMsg = err.ErrorMessage();
        QString errMsgStr = (LPSTR)errMsg;
        emit publishEventMsg(tr("WindowsFirewallIsOn failed: 0x%1lx").arg(gkStringFuncs->zeroPadding(errMsgStr, GK_MICROSOFT_HR_ERROR_MSG_ZERO_PADDING)), GkSeverity::Fatal, "", false, true, false, true);
        goto error;
    }

    // If it is, turn it on.
    if (!fwOn) {
        // Turn the firewall on.
        hr = fwProfile->put_FirewallEnabled(VARIANT_TRUE);
        if (FAILED(hr)) {
            _com_error err(hr);
            LPCTSTR errMsg = err.ErrorMessage();
            QString errMsgStr = (LPSTR)errMsg;
            emit publishEventMsg(tr("put_FirewallEnabled failed: 0x%1lx").arg(gkStringFuncs->zeroPadding(errMsgStr, GK_MICROSOFT_HR_ERROR_MSG_ZERO_PADDING)), GkSeverity::Fatal, "", false, true, false, true);
            goto error;
        }

        emit publishEventMsg(tr("The firewall is now on."), GkSeverity::Debug, "", true, true, false, false);
    }

    error:
    return hr;
}
#endif

#if defined(_WIN32) || defined(__MINGW64__) || defined(__CYGWIN__)
/**
 * @brief GkSystem::WindowsFirewallTurnOff
 * @author Microsoft Corporation <https://docs.microsoft.com/en-us/previous-versions//aa364726(v=vs.85)?redirectedfrom=MSDN>,
 * Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param fwProfile
 * @return
 * @note Exercising the Firewall using C++ <https://docs.microsoft.com/en-us/previous-versions//aa364726(v=vs.85)?redirectedfrom=MSDN>.
 */
HRESULT GkSystem::WindowsFirewallTurnOff(INetFwProfile *fwProfile)
{
    HRESULT hr = S_OK;
    BOOL fwOn;

    Q_ASSERT(fwProfile != nullptr);

    // Check to see if the firewall is on.
    hr = WindowsFirewallIsOn(fwProfile, &fwOn);
    if (FAILED(hr)) {
        _com_error err(hr);
        LPCTSTR errMsg = err.ErrorMessage();
        QString errMsgStr = (LPSTR)errMsg;
        emit publishEventMsg(tr("WindowsFirewallIsOn failed: 0x%1lx").arg(gkStringFuncs->zeroPadding(errMsgStr, GK_MICROSOFT_HR_ERROR_MSG_ZERO_PADDING)), GkSeverity::Fatal, "", false, true, false, true);
        goto error;
    }

    // If it is, turn it off.
    if (fwOn) {
        // Turn the firewall off.
        hr = fwProfile->put_FirewallEnabled(VARIANT_FALSE);
        if (FAILED(hr)) {
            _com_error err(hr);
            LPCTSTR errMsg = err.ErrorMessage();
            QString errMsgStr = (LPSTR)errMsg;
            emit publishEventMsg(tr("put_FirewallEnabled failed: 0x%1lx").arg(gkStringFuncs->zeroPadding(errMsgStr, GK_MICROSOFT_HR_ERROR_MSG_ZERO_PADDING)), GkSeverity::Fatal, "", false, true, false, true);
            goto error;
        }

        emit publishEventMsg(tr("The firewall is now off."), GkSeverity::Debug, "", true, true, false, false);
    }

    error:
    return hr;
}
#endif

#if defined(_WIN32) || defined(__MINGW64__) || defined(__CYGWIN__)
/**
 * @brief GkSystem::WindowsFirewallAddApp
 * @author Microsoft Corporation <https://docs.microsoft.com/en-us/previous-versions//aa364726(v=vs.85)?redirectedfrom=MSDN>,
 * Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param fwProfile
 * @param fwProcessImageFileName
 * @param fwName
 * @return
 * @note Exercising the Firewall using C++ <https://docs.microsoft.com/en-us/previous-versions//aa364726(v=vs.85)?redirectedfrom=MSDN>.
 */
HRESULT GkSystem::WindowsFirewallAddApp(INetFwProfile *fwProfile, const wchar_t *fwProcessImageFileName, const wchar_t *fwName)
{
    HRESULT hr = S_OK;
    BOOL fwAppEnabled;
    BSTR fwBstrName = nullptr;
    BSTR fwBstrProcessImageFileName = nullptr;
    INetFwAuthorizedApplication *fwApp = nullptr;
    INetFwAuthorizedApplications *fwApps = nullptr;

    Q_ASSERT(fwProfile != nullptr);
    Q_ASSERT(fwProcessImageFileName != nullptr);
    Q_ASSERT(fwName != nullptr);

    // First check to see if the application is already authorized.
    hr = WindowsFirewallAppIsEnabled(fwProfile, fwProcessImageFileName, &fwAppEnabled);
    if (FAILED(hr)) {
        _com_error err(hr);
        LPCTSTR errMsg = err.ErrorMessage();
        QString errMsgStr = (LPSTR)errMsg;
        emit publishEventMsg(tr("WindowsFirewallAppIsEnabled failed: 0x%1lx").arg(gkStringFuncs->zeroPadding(errMsgStr, GK_MICROSOFT_HR_ERROR_MSG_ZERO_PADDING)), GkSeverity::Fatal, "", false, true, false, true);

        goto error;
    }

    // Only add the application if it isn't already authorized.
    if (!fwAppEnabled) {
        // Retrieve the authorized application collection.
        hr = fwProfile->get_AuthorizedApplications(&fwApps);
        if (FAILED(hr)) {
            _com_error err(hr);
            LPCTSTR errMsg = err.ErrorMessage();
            QString errMsgStr = (LPSTR)errMsg;
            emit publishEventMsg(tr("get_AuthorizedApplications failed: 0x%1lx").arg(gkStringFuncs->zeroPadding(errMsgStr, GK_MICROSOFT_HR_ERROR_MSG_ZERO_PADDING)), GkSeverity::Fatal, "", false, true, false, true);

            goto error;
        }

        // Create an instance of an authorized application.
        hr = CoCreateInstance(__uuidof(NetFwAuthorizedApplication), nullptr, CLSCTX_INPROC_SERVER, __uuidof(INetFwAuthorizedApplication), (void**)&fwApp);
        if (FAILED(hr)) {
            _com_error err(hr);
            LPCTSTR errMsg = err.ErrorMessage();
            QString errMsgStr = (LPSTR)errMsg;
            emit publishEventMsg(tr("CoCreateInstance failed: 0x%1lx").arg(gkStringFuncs->zeroPadding(errMsgStr, GK_MICROSOFT_HR_ERROR_MSG_ZERO_PADDING)), GkSeverity::Fatal, "", false, true, false, true);

            goto error;
        }

        // Allocate a BSTR for the process image file name.
        fwBstrProcessImageFileName = SysAllocString(fwProcessImageFileName);
        if (fwBstrProcessImageFileName == nullptr) {
            hr = E_OUTOFMEMORY;
            _com_error err(hr);
            LPCTSTR errMsg = err.ErrorMessage();
            QString errMsgStr = (LPSTR)errMsg;
            emit publishEventMsg(tr("SysAllocString failed: 0x%1lx").arg(gkStringFuncs->zeroPadding(errMsgStr, GK_MICROSOFT_HR_ERROR_MSG_ZERO_PADDING)), GkSeverity::Fatal, "", false, true, false, true);

            goto error;
        }

        // Set the process image file name.
        hr = fwApp->put_ProcessImageFileName(fwBstrProcessImageFileName);
        if (FAILED(hr)) {
            _com_error err(hr);
            LPCTSTR errMsg = err.ErrorMessage();
            QString errMsgStr = (LPSTR)errMsg;
            emit publishEventMsg(tr("put_ProcessImageFileName failed: 0x%1lx").arg(gkStringFuncs->zeroPadding(errMsgStr, GK_MICROSOFT_HR_ERROR_MSG_ZERO_PADDING)), GkSeverity::Fatal, "", false, true, false, true);

            goto error;
        }

        // Allocate a BSTR for the application friendly name.
        fwBstrName = SysAllocString(fwName);
        if (SysStringLen(fwBstrName) == 0) {
            hr = E_OUTOFMEMORY;
            _com_error err(hr);
            LPCTSTR errMsg = err.ErrorMessage();
            QString errMsgStr = (LPSTR)errMsg;
            emit publishEventMsg(tr("SysAllocString failed: 0x%1lx").arg(gkStringFuncs->zeroPadding(errMsgStr, GK_MICROSOFT_HR_ERROR_MSG_ZERO_PADDING)), GkSeverity::Fatal, "", false, true, false, true);

            goto error;
        }

        // Set the application friendly name.
        hr = fwApp->put_Name(fwBstrName);
        if (FAILED(hr)) {
            _com_error err(hr);
            LPCTSTR errMsg = err.ErrorMessage();
            QString errMsgStr = (LPSTR)errMsg;
            emit publishEventMsg(tr("put_Name failed: 0x%1lx").arg(gkStringFuncs->zeroPadding(errMsgStr, GK_MICROSOFT_HR_ERROR_MSG_ZERO_PADDING)), GkSeverity::Fatal, "", false, true, false, true);

            goto error;
        }

        // Add the application to the collection.
        hr = fwApps->Add(fwApp);
        if (FAILED(hr)) {
            _com_error err(hr);
            LPCTSTR errMsg = err.ErrorMessage();
            QString errMsgStr = (LPSTR)errMsg;
            emit publishEventMsg(tr("Add failed: 0x%1lx").arg(gkStringFuncs->zeroPadding(errMsgStr, GK_MICROSOFT_HR_ERROR_MSG_ZERO_PADDING)), GkSeverity::Fatal, "", false, true, false, true);

            goto error;
        }

        emit publishEventMsg(tr("Authorized application %1 is now enabled in the firewall.").arg(QString::fromWCharArray(fwProcessImageFileName)), GkSeverity::Debug, "", true, true, false, false);
    }

    error:

    // Free the BSTRs.
    SysFreeString(fwBstrName);
    SysFreeString(fwBstrProcessImageFileName);

    // Release the authorized application instance.
    if (fwApp != nullptr) {
        fwApp->Release();
    }

    // Release the authorized application collection.
    if (fwApps != nullptr) {
        fwApps->Release();
    }

    return hr;
}
#endif

#if defined(_WIN32) || defined(__MINGW64__) || defined(__CYGWIN__)
/**
 * @brief GkSystem::WindowsFirewallPortIsEnabled
 * @author Microsoft Corporation <https://docs.microsoft.com/en-us/previous-versions//aa364726(v=vs.85)?redirectedfrom=MSDN>,
 * Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param fwProfile
 * @param portNumber
 * @param ipProtocol
 * @param fwPortEnabled
 * @return
 * @note Exercising the Firewall using C++ <https://docs.microsoft.com/en-us/previous-versions//aa364726(v=vs.85)?redirectedfrom=MSDN>.
 */
HRESULT GkSystem::WindowsFirewallPortIsEnabled(INetFwProfile *fwProfile, LONG portNumber, NET_FW_IP_PROTOCOL ipProtocol, WINBOOL *fwPortEnabled)
{
    HRESULT hr = S_OK;
    VARIANT_BOOL fwEnabled;
    INetFwOpenPort *fwOpenPort = nullptr;
    INetFwOpenPorts *fwOpenPorts = nullptr;

    Q_ASSERT(fwProfile != nullptr);
    Q_ASSERT(fwPortEnabled != nullptr);

    *fwPortEnabled = FALSE;

    // Retrieve the globally open ports collection.
    hr = fwProfile->get_GloballyOpenPorts(&fwOpenPorts);
    if (FAILED(hr)) {
        _com_error err(hr);
        LPCTSTR errMsg = err.ErrorMessage();
        QString errMsgStr = (LPSTR)errMsg;
        emit publishEventMsg(tr("get_GloballyOpenPorts failed: 0x%1lx").arg(gkStringFuncs->zeroPadding(errMsgStr, GK_MICROSOFT_HR_ERROR_MSG_ZERO_PADDING)), GkSeverity::Fatal, "", false, true, false, true);

        goto error;
    }

    // Attempt to retrieve the globally open port.
    hr = fwOpenPorts->Item(portNumber, ipProtocol, &fwOpenPort);
    if (SUCCEEDED(hr)) {
        // Find out if the globally open port is enabled.
        hr = fwOpenPort->get_Enabled(&fwEnabled);
        if (FAILED(hr)) {
            _com_error err(hr);
            LPCTSTR errMsg = err.ErrorMessage();
            QString errMsgStr = (LPSTR)errMsg;
            emit publishEventMsg(tr("get_Enabled failed: 0x%1lx").arg(gkStringFuncs->zeroPadding(errMsgStr, GK_MICROSOFT_HR_ERROR_MSG_ZERO_PADDING)), GkSeverity::Fatal, "", false, true, false, true);

            goto error;
        }

        if (fwEnabled != VARIANT_FALSE) {
            // The globally open port is enabled.
            *fwPortEnabled = TRUE;
            emit publishEventMsg(tr("Port %1 is open in the firewall.").arg(QString::number(portNumber)), GkSeverity::Debug, "", true, true, false, false);
        } else {
            emit publishEventMsg(tr("Port %1 is not open in the firewall.").arg(QString::number(portNumber)), GkSeverity::Debug, "", true, true, false, false);
        }
    } else {
        // The globally open port was not in the collection.
        hr = S_OK;
        emit publishEventMsg(tr("Port %1 is not open in the firewall.").arg(QString::number(portNumber)), GkSeverity::Debug, "", true, true, false, false);
    }

    error:

    // Release the globally open port.
    if (fwOpenPort != nullptr) {
        fwOpenPort->Release();
    }

    // Release the globally open ports collection.
    if (fwOpenPorts != nullptr) {
        fwOpenPorts->Release();
    }

    return hr;
}
#endif

#if defined(_WIN32) || defined(__MINGW64__) || defined(__CYGWIN__)
/**
 * @brief GkSystem::WindowsFirewallPortAdd
 * @author Microsoft Corporation <https://docs.microsoft.com/en-us/previous-versions//aa364726(v=vs.85)?redirectedfrom=MSDN>,
 * Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param fwProfile
 * @param portNumber
 * @param ipProtocol
 * @param name
 * @return
 * @note Exercising the Firewall using C++ <https://docs.microsoft.com/en-us/previous-versions//aa364726(v=vs.85)?redirectedfrom=MSDN>.
 */
HRESULT GkSystem::WindowsFirewallPortAdd(INetFwProfile *fwProfile, LONG portNumber, NET_FW_IP_PROTOCOL ipProtocol, const wchar_t *name)
{
    HRESULT hr = S_OK;
    BOOL fwPortEnabled;
    BSTR fwBstrName = nullptr;
    INetFwOpenPort *fwOpenPort = nullptr;
    INetFwOpenPorts *fwOpenPorts = nullptr;

    Q_ASSERT(fwProfile != nullptr);
    Q_ASSERT(name != nullptr);

    // First check to see if the port is already added.
    hr = WindowsFirewallPortIsEnabled(fwProfile, portNumber, ipProtocol, &fwPortEnabled);
    if (FAILED(hr)) {
        _com_error err(hr);
        LPCTSTR errMsg = err.ErrorMessage();
        QString errMsgStr = (LPSTR)errMsg;
        emit publishEventMsg(tr("WindowsFirewallPortIsEnabled failed: 0x%1lx").arg(gkStringFuncs->zeroPadding(errMsgStr, GK_MICROSOFT_HR_ERROR_MSG_ZERO_PADDING)), GkSeverity::Fatal, "", false, true, false, true);

        goto error;
    }

    // Only add the port if it isn't already added.
    if (!fwPortEnabled) {
        // Retrieve the collection of globally open ports.
        hr = fwProfile->get_GloballyOpenPorts(&fwOpenPorts);
        if (FAILED(hr)) {
            _com_error err(hr);
            LPCTSTR errMsg = err.ErrorMessage();
            QString errMsgStr = (LPSTR)errMsg;
            emit publishEventMsg(tr("get_GloballyOpenPorts failed: 0x%1lx").arg(gkStringFuncs->zeroPadding(errMsgStr, GK_MICROSOFT_HR_ERROR_MSG_ZERO_PADDING)), GkSeverity::Fatal, "", false, true, false, true);

            goto error;
        }

        // Create an instance of an open port.
        hr = CoCreateInstance(__uuidof(NetFwOpenPort), nullptr, CLSCTX_INPROC_SERVER, __uuidof(INetFwOpenPort), (void**)&fwOpenPort);
        if (FAILED(hr)) {
            _com_error err(hr);
            LPCTSTR errMsg = err.ErrorMessage();
            QString errMsgStr = (LPSTR)errMsg;
            emit publishEventMsg(tr("CoCreateInstance failed: 0x%1lx").arg(gkStringFuncs->zeroPadding(errMsgStr, GK_MICROSOFT_HR_ERROR_MSG_ZERO_PADDING)), GkSeverity::Fatal, "", false, true, false, true);

            goto error;
        }

        // Set the port number.
        hr = fwOpenPort->put_Port(portNumber);
        if (FAILED(hr)) {
            _com_error err(hr);
            LPCTSTR errMsg = err.ErrorMessage();
            QString errMsgStr = (LPSTR)errMsg;
            emit publishEventMsg(tr("put_Port failed: 0x%1lx").arg(gkStringFuncs->zeroPadding(errMsgStr, GK_MICROSOFT_HR_ERROR_MSG_ZERO_PADDING)), GkSeverity::Fatal, "", false, true, false, true);

            goto error;
        }

        // Set the IP protocol.
        hr = fwOpenPort->put_Protocol(ipProtocol);
        if (FAILED(hr)) {
            _com_error err(hr);
            LPCTSTR errMsg = err.ErrorMessage();
            QString errMsgStr = (LPSTR)errMsg;
            emit publishEventMsg(tr("put_Protocol failed: 0x%1lx").arg(gkStringFuncs->zeroPadding(errMsgStr, GK_MICROSOFT_HR_ERROR_MSG_ZERO_PADDING)), GkSeverity::Fatal, "", false, true, false, true);

            goto error;
        }

        // Allocate a BSTR for the friendly name of the port.
        fwBstrName = SysAllocString(name);
        if (SysStringLen(fwBstrName) == 0) {
            hr = E_OUTOFMEMORY;
            _com_error err(hr);
            LPCTSTR errMsg = err.ErrorMessage();
            QString errMsgStr = (LPSTR)errMsg;
            emit publishEventMsg(tr("SysAllocString failed: 0x%1lx").arg(gkStringFuncs->zeroPadding(errMsgStr, GK_MICROSOFT_HR_ERROR_MSG_ZERO_PADDING)), GkSeverity::Fatal, "", false, true, false, true);

            goto error;
        }

        // Set the friendly name of the port.
        hr = fwOpenPort->put_Name(fwBstrName);
        if (FAILED(hr)) {
            _com_error err(hr);
            LPCTSTR errMsg = err.ErrorMessage();
            QString errMsgStr = (LPSTR)errMsg;
            emit publishEventMsg(tr("put_Name failed: 0x%1lx").arg(gkStringFuncs->zeroPadding(errMsgStr, GK_MICROSOFT_HR_ERROR_MSG_ZERO_PADDING)), GkSeverity::Fatal, "", false, true, false, true);

            goto error;
        }

        // Opens the port and adds it to the collection.
        hr = fwOpenPorts->Add(fwOpenPort);
        if (FAILED(hr)) {
            _com_error err(hr);
            LPCTSTR errMsg = err.ErrorMessage();
            QString errMsgStr = (LPSTR)errMsg;
            emit publishEventMsg(tr("Add failed: 0x%1lx").arg(gkStringFuncs->zeroPadding(errMsgStr, GK_MICROSOFT_HR_ERROR_MSG_ZERO_PADDING)), GkSeverity::Fatal, "", false, true, false, true);

            goto error;
        }

        emit publishEventMsg(tr("Port %1 is now open in the firewall.").arg(QString::number(portNumber)), GkSeverity::Debug, "", true, true, false, false);
    }

    error:

    // Free the BSTR.
    SysFreeString(fwBstrName);

    // Release the open port instance.
    if (fwOpenPort != nullptr) {
        fwOpenPort->Release();
    }

    // Release the globally open ports collection.
    if (fwOpenPorts != nullptr) {
        fwOpenPorts->Release();
    }

    return hr;
}
#endif
