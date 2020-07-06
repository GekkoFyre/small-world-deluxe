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

#include "gk_string_funcs.hpp"
#include <QSettings>

#if _WIN32
#include <stringapiset.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(__linux__) || defined(__MINGW64__)
#include <SDL2/SDL_video.h>
#endif

#ifdef __cplusplus
} // extern "C"
#endif

using namespace GekkoFyre;

StringFuncs::StringFuncs(QObject *parent) : QObject(parent)
{}

StringFuncs::~StringFuncs()
{}

#if defined(_MSC_VER) && (_MSC_VER > 1900)
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

/**
 * @brief StringFuncs::modalDlgBoxOk Creates a modal message box within the Win32 API, with an OK button.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @note <https://docs.microsoft.com/en-us/windows/win32/dlgbox/using-dialog-boxes#creating-a-modal-dialog-box>
 * <https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-messagebox>
 * @param hwnd
 * @param title
 * @param msgTxt
 * @param icon
 * @return Whether the OK button was selected or not.
 * @see GekkoFyre::PaAudioBuf::dlgBoxOk().
 */

bool StringFuncs::modalDlgBoxOk(const HWND &hwnd, const QString &title, const QString &msgTxt, const int &icon)
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
bool StringFuncs::modalDlgBoxLinux(uint32_t flags, const QString &title, const QString &msgTxt)
{
    SDL_Window *sdlWindow = SDL_CreateWindow(General::productName, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, DLG_BOX_WINDOW_WIDTH, DLG_BOX_WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    int ret = SDL_ShowSimpleMessageBox(flags, title.toStdString().c_str(), msgTxt.toStdString().c_str(), sdlWindow);
    SDL_DestroyWindow(sdlWindow);
    return ret;
}
#endif
