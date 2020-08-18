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

#pragma once

#include "src/defines.hpp"
#include <QObject>
#include <QMessageBox>
#include <QString>
#include <string>
#include <memory>
#include <vector>
#include <mutex>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace GekkoFyre {

class StringFuncs : public QObject {
    Q_OBJECT

public:
    explicit StringFuncs(QObject *parent = nullptr);
    ~StringFuncs() override;

    #if defined(_MSC_VER) && (_MSC_VER > 1900)
    static std::string multiByteFromWide(LPCWSTR pwsz, UINT cp);
    static std::wstring strToWStrWin(const std::string &s);
    std::wstring removeSpecialChars(std::wstring wstr);
    bool modalDlgBoxOk(const HWND &hwnd, const QString &title, const QString &msgTxt, const int &icon);
    #endif

    QString getStringFromUnsignedChar(unsigned char *str);
    std::vector<int> convStrToIntArray(const QString &str);

};
};
