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

#include "src/gk_system.hpp"
#include <exception>
#include <utility>
#include <QMessageBox>
#include <QtGlobal>

#if defined(_WIN32) || defined(__MINGW64__)
// Forward declarations
HRESULT WFCOMInitialize(INetFwPolicy2** ppNetFwPolicy2);
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

    #if defined(_WIN32) || defined(__MINGW64__)
    #elif __linux__
    ss << "/dev/";
    #endif

    switch (conn_type) {
        case GkRS232:
        {
            #if defined(_WIN32) || defined(__MINGW64__)
            #elif __linux__
            ss << "ttyS" << port;
            #endif
            break;
        }
        case GkUSB:
        {
            #if defined(_WIN32) || defined(__MINGW64__)
            #elif __linux__
            ss << "ttyUSB" << port;
            #endif
            break;
        }
        case GkParallel:
        {
            #if defined(_WIN32) || defined(__MINGW64__)
            #elif __linux__
            ss << "parport" << port;
            #endif
            break;
        }
        case GkGPIO:
        {
            #if defined(_WIN32) || defined(__MINGW64__)
            #elif __linux__
            ss.clear();
            ss << "/sys/class/gpio/gpio" << port;
            #endif
            break;
        }
        case GkCM108:
            throw std::invalid_argument(tr("CM108 is currently not supported by %1!").arg(General::productName).toStdString());
        case GkNone:
            return "";
        default:
            return "";
    }

    return QString::fromStdString(ss.str());
}

#if defined(_WIN32) || defined(__MINGW64__)
/**
 * @brief GkSystem::addPolicyToWindowsFirewallApi adds an outbound rule to the Microsoft Windows firewall, provided it's activated, and it
 * does this via the official C++ API.
 * @author Microsoft <https://docs.microsoft.com/en-us/previous-versions/windows/desktop/ics/c-adding-an-outbound-rule>,
 * Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkSystem::addPolicyToWindowsFirewallApi()
{
    HRESULT hrComInit = S_OK;
    HRESULT hr = S_OK;

    INetFwPolicy2 *pNetFwPolicy2 = nullptr;
    INetFwRules *pFwRules = nullptr;
    INetFwRule *pFwRule = nullptr;

    long CurrentProfilesBitMask = 0;

    QString programFilesPath(getenv("PROGRAMFILES"));

    BSTR bstrRuleName = SysAllocString(L"OUTBOUND_RULE");
    BSTR bstrRuleDescription = gkStringFuncs->convQStringToWinBStr(tr("Allow outbound network traffic from %1 over TCP port 443 towards destination, [ %2 ].")
            .arg(General::productName).arg(General::gk_sentry_user_side_uri));
    BSTR bstrRuleGroup = SysAllocString(L(General::companyName));
    BSTR bstrRuleApplication = gkStringFuncs->convQStringToWinBStr(tr("%1\\%2\\%3.exe")
            .arg(programFilesPath).arg(General::productName).arg(General::executableName));
    BSTR bstrRuleLPorts = SysAllocString(L"443");

    // Initialize COM.
    hrComInit = CoInitializeEx(0, COINIT_APARTMENTTHREADED);

    // Ignore RPC_E_CHANGED_MODE; this just means that COM has already been
    // initialized with a different mode. Since we don't care what the mode is,
    // we'll just use the existing mode.
    if (hrComInit != RPC_E_CHANGED_MODE) {
        if (FAILED(hrComInit)) {
            gkEventLogger->publishEvent(tr("CoInitializeEx failed: 0x%08lx").arg(QString::number(hrComInit)), GkSeverity::Error, "", false, true);
            goto Cleanup;
        }
    }

    // Retrieve INetFwPolicy2
    hr = WFCOMInitialize(&amp;pNetFwPolicy2);
    if (FAILED(hr)) {
        goto Cleanup;
    }

    // Retrieve INetFwRules
    hr = pNetFwPolicy2->get_Rules(&amp;pFwRules);
    if (FAILED(hr)) {
        gkEventLogger->publishEvent(tr("get_Rules failed: 0x%08lx").arg(QString::number(hr)), GkSeverity::Error, "", false, true);
        goto Cleanup;
    }

    // Retrieve Current Profiles bitmask
    hr = pNetFwPolicy2->get_CurrentProfileTypes(&amp;CurrentProfilesBitMask);
    if (FAILED(hr)) {
        gkEventLogger->publishEvent(tr("get_CurrentProfileTypes failed: 0x%08lx").arg(QString::number(hr)), GkSeverity::Error, "", false, true);
        goto Cleanup;
    }

    // When possible we avoid adding firewall rules to the Public profile.
    // If Public is currently active and it is not the only active profile, we remove it from the bitmask
    if ((CurrentProfilesBitMask & NET_FW_PROFILE2_PUBLIC) (CurrentProfilesBitMask != NET_FW_PROFILE2_PUBLIC)) {
        CurrentProfilesBitMask ^= NET_FW_PROFILE2_PUBLIC;
    }

    // Create a new Firewall Rule object.
    hr = CoCreateInstance(__uuidof(NetFwRule), NULL, CLSCTX_INPROC_SERVER, __uuidof(INetFwRule), (void**)&amp;pFwRule);
    if (FAILED(hr)) {
        gkEventLogger->publishEvent(tr("CoCreateInstance for Firewall Rule failed: 0x%08lx").arg(QString::number(hr)), GkSeverity::Error, "", true, true);
        goto Cleanup;
    }

    // Populate the Firewall Rule object
    pFwRule->put_Name(bstrRuleName);
    pFwRule->put_Description(bstrRuleDescription);
    pFwRule->put_ApplicationName(bstrRuleApplication);
    pFwRule->put_Protocol(NET_FW_IP_PROTOCOL_TCP);
    pFwRule->put_LocalPorts(bstrRuleLPorts);
    pFwRule->put_Direction(NET_FW_RULE_DIR_OUT);
    pFwRule->put_Grouping(bstrRuleGroup);
    pFwRule->put_Profiles(CurrentProfilesBitMask);
    pFwRule->put_Action(NET_FW_ACTION_ALLOW);
    pFwRule->put_Enabled(VARIANT_TRUE);

    // Add the Firewall Rule
    hr = pFwRules->Add(pFwRule);
    if (FAILED(hr)) {
        gkEventLogger->publishEvent(tr("Firewall Rule Add failed: 0x%08lx").arg(QString::number(hr)), GkSeverity::Error, "", true, true);
        goto Cleanup;
    }

    Cleanup:

    // Free BSTR's
    SysFreeString(bstrRuleName);
    SysFreeString(bstrRuleDescription);
    SysFreeString(bstrRuleGroup);
    SysFreeString(bstrRuleApplication);
    SysFreeString(bstrRuleLPorts);

    // Release the INetFwRule object
    if (pFwRule != NULL) {
        pFwRule->Release();
    }

    // Release the INetFwRules object
    if (pFwRules != NULL) {
        pFwRules->Release();
    }

    // Release the INetFwPolicy2 object
    if (pNetFwPolicy2 != NULL) {
        pNetFwPolicy2->Release();
    }

    // Uninitialize COM.
    if (SUCCEEDED(hrComInit)) {
        CoUninitialize();
    }

    return;
}
#endif

#if defined(_WIN32) || defined(__MINGW64__)
/**
 * @brief GkSystem::isWindowsFirewallEnabled will detect whether the Microsoft Windows firewall is enabled or
 * not, programmatically, via the Microsoft Windows C++ API.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param INetFwProfile
 * @param fwOn
 * @return
 * @note <https://docs.microsoft.com/en-us/previous-versions//aa364726(v=vs.85)>
 * <https://docs.microsoft.com/en-us/previous-versions/windows/desktop/ics/c-adding-an-outbound-rule>
 */
GkSystem::isWindowsFirewallEnabled(IN INetFwProfile *fwProfile, OUT BOOL *fwOn)
{
    return;
}
#endif
