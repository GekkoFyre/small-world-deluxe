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

#include "string_funcs_windows.hpp"

#ifdef __cplusplus
extern "C"
{
#endif
#ifdef _WIN32
#include <stringapiset.h>
#endif
#ifdef __cplusplus
}
#endif

using namespace GekkoFyre;

StringFuncs::StringFuncs(QObject *parent) : QObject(parent)
{}

StringFuncs::~StringFuncs()
{}

/**
 * @brief StringFuncs::multiByteFromWide Converts a widestring to a multibyte string, when concerning Microsoft Windows
 * C/C++ related code/functions.
 * @author Jon <https://stackoverflow.com/questions/5513718/how-do-i-convert-from-lpctstr-to-stdstring>
 * Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param pwsz The widestring in question that is to be converted.
 * @param cp
 * @return The converted multibyte string as output.
 */
std::string StringFuncs::multiByteFromWide(LPCWSTR pwsz, UINT cp)
{
    int cch = WideCharToMultiByte(cp, 0, pwsz, -1, nullptr, 0, nullptr, nullptr);
    char *psz = new char[cch];
    WideCharToMultiByte(cp, 0, pwsz, -1, psz, cch, nullptr, nullptr);

    std::string str(psz);
    delete[] psz;

    return str;
}

/**
 * @brief StringFuncs::strToWStrWin Converts a `std::string` towards a `LPCWSTR`, specifically for Microsoft Windows systems.
 * @author Toran Billups <https://stackoverflow.com/questions/27220/how-to-convert-stdstring-to-lpcwstr-in-c-unicode>
 * @param s The `std::string` variable to be converted.
 * @return The converted `std::wstring` variable.
 */
std::wstring StringFuncs::strToWStrWin(const std::string &s)
{
    int len;
    int slength = ((int)s.length() + 1);
    len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, nullptr, 0);

    wchar_t *buf = new wchar_t[len];
    MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
    std::wstring r(buf);

    delete[] buf;
    return r;
}

/**
 * @brief StringFuncs::removeSpecialChars
 * @param str
 * @return
 */
std::wstring StringFuncs::removeSpecialChars(std::wstring wstr)
{
    wstr.erase(std::remove_if(wstr.begin(), wstr.end(), [](char ch){ return !::iswalnum(ch); }), wstr.end());
    return wstr;
}
