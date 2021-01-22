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

#pragma once

#include "src/defines.hpp"
#include "src/gk_string_funcs.hpp"
#include "src/gk_logger.hpp"
#include <string>
#include <cstdio>
#include <QObject>
#include <QString>
#include <QPointer>

#if defined(_WIN32) || defined(__MINGW64__) || defined(__CYGWIN__)
#include <windows.h>
#include <crtdbg.h>
#include <netfw.h>
#include <objbase.h>
#include <oleauto.h>
#endif

namespace GekkoFyre {

class GkSystem : public QObject {
    Q_OBJECT

public:
    explicit GkSystem(QPointer<GekkoFyre::StringFuncs> stringFuncs, QObject *parent = nullptr);
    ~GkSystem() override;

    qint32 getNumCpuCores();
    QString renameCommsDevice(const qint32 &port, const GekkoFyre::AmateurRadio::GkConnType &conn_type);

    #if defined(_WIN32) || defined(__MINGW64__) || defined(__CYGWIN__)
    HRESULT WindowsFirewallInitialize(OUT INetFwProfile **fwProfile);
    HRESULT WindowsFirewallIsOn(IN INetFwProfile *fwProfile, OUT BOOL *fwOn);
    HRESULT WindowsFirewallAppIsEnabled(IN INetFwProfile *fwProfile, IN const wchar_t *fwProcessImageFileName, OUT BOOL *fwAppEnabled);
    HRESULT WindowsFirewallTurnOn(IN INetFwProfile *fwProfile);
    HRESULT WindowsFirewallTurnOff(IN INetFwProfile *fwProfile);
    HRESULT WindowsFirewallAddApp(IN INetFwProfile *fwProfile, IN const wchar_t *fwProcessImageFileName, IN const wchar_t *fwName);
    HRESULT WindowsFirewallPortIsEnabled(IN INetFwProfile *fwProfile, IN LONG portNumber, IN NET_FW_IP_PROTOCOL ipProtocol, OUT BOOL *fwPortEnabled);
    HRESULT WindowsFirewallPortAdd(IN INetFwProfile *fwProfile, IN LONG portNumber, IN NET_FW_IP_PROTOCOL ipProtocol, IN const wchar_t *name);

    static QString processHResult(const HRESULT &hr);
    #endif

signals:
    void publishEventMsg(const QString &event, const GekkoFyre::System::Events::Logging::GkSeverity &severity = GekkoFyre::System::Events::Logging::GkSeverity::Warning,
                         const QVariant &arguments = "", const bool &sys_notification = false, const bool &publishToConsole = true, const bool &publishToStatusBar = false,
                         const bool &displayMsgBox = false);

private:
    QPointer<GekkoFyre::StringFuncs> gkStringFuncs;

};
};
