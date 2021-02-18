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

#include "src/gk_string_funcs.hpp"
#include <boost/exception/all.hpp>
#include <QRegularExpression>
#include <QMessageBox>
#include <exception>
#include <QSettings>
#include <algorithm>
#include <iterator>
#include <sstream>
#include <random>

#if _WIN32
#include <windows.h>
#include <stringapiset.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#if _WIN32
#elif MACOS
#include <sys/param.h>
#include <sys/sysctl.h>
#else
#include <unistd.h>
#endif

#ifdef __cplusplus
} // extern "C"
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

StringFuncs::StringFuncs(QObject *parent) : QObject(parent)
{}

StringFuncs::~StringFuncs()
{}

void StringFuncs::print_exception(const std::exception &e, int level)
{
    QMessageBox::warning(nullptr, tr("Error!"), e.what(), QMessageBox::Ok);

    try {
        std::rethrow_if_nested(e);
    } catch (const std::exception &e) {
        print_exception(e, level + 1);
    } catch (...) {}

    return;
}

/**
 * @brief StringFuncs::getStringFromUnsignedChar
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param str
 * @return
 */
QString StringFuncs::getStringFromUnsignedChar(unsigned char *str)
{
    QString qstr = QString::fromUtf8(reinterpret_cast<const char *>(str));
    return qstr;
}

std::vector<int> StringFuncs::convStrToIntArray(const QString &str)
{
    const std::string buffer = str.toStdString();
    std::vector<int> int_arr(buffer.size());
    std::copy(buffer.begin(), buffer.end(), int_arr.begin());

    return int_arr;
}

/**
 * @brief StringFuncs::addErrorMsg
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param orig_msg
 * @param err_msg
 * @return
 */
QString StringFuncs::addErrorMsg(const QString &orig_msg, const QString &err_msg)
{
    if (!orig_msg.isNull() && !orig_msg.isEmpty()) {
        if (!err_msg.isNull() && !err_msg.isEmpty()) {
            QString orig_msg_modified = orig_msg;
            orig_msg_modified += tr(" Error:\n\n");
            orig_msg_modified += err_msg;

            return orig_msg_modified;
        }
    }

    return QString();
}

/**
 * @brief StringFuncs::csvSplitter splits up a given string of CSV elements, using ',' as the delimiter, and outputs it
 * as a std::vector<std::string> ready for use and modification by other functions.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param csv_vals
 * @return
 */
std::deque<std::string> StringFuncs::csvSplitter(const std::string &csv_vals)
{
    try {
        if (!csv_vals.empty()) {
            std::stringstream ss(csv_vals);
            std::deque<std::string> result;
            while (ss.good()) {
                std::string substr;
                std::getline(ss, substr, ',');
                if (!substr.empty()) {
                    result.push_back(substr);
                }
            }

            return result;
        }
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(tr("An error has occurred whilst attempting to modify CSV data.\n\n%1").arg(QString::fromStdString(e.what())).toStdString()));
    }

    return std::deque<std::string>();
}

/**
 * @brief StringFuncs::csvRemoveElement removes a given std::string element from a std::vector<std::string> list, which is
 * particularly useful when combined with the function, StringFuncs::csvSplitter().
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param csv_elements
 * @param val_to_remove
 * @return
 * @see StringFuncs::csvSplitter().
 */
std::deque<std::string> StringFuncs::csvRemoveElement(const std::deque<std::string> &csv_elements, const std::string &val_to_remove)
{
    try {
        if (!csv_elements.empty()) {
            std::deque<std::string> csv_elements_cpy(csv_elements.begin(), csv_elements.end());
            for (auto it = csv_elements_cpy.begin(); it != csv_elements_cpy.end();) {
                if (*it == val_to_remove) {
                    it = csv_elements_cpy.erase(it);
                    return csv_elements_cpy;
                } else {
                    ++it;
                }
            }
        }
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(tr("An error has occurred whilst attempting to modify CSV data.\n\n%1").arg(QString::fromStdString(e.what())).toStdString()));
    }

    return std::deque<std::string>();
}

/**
 * @brief StringFuncs::csvOutputString saves a given std::vector<std::string> list as an std::string in a CSV-like
 * manner, which is particularly useful when combined with the StringFuncs::csvSplitter() function after having made
 * modifications to the std::vector<std::string> elements themselves with other functions such as StringFuncs::csvRemoveElement().
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param csv_elements
 * @return
 * @see StringFuncs::csvSplitter(), StringFuncs::csvRemoveElement().
 */
std::string StringFuncs::csvOutputString(const std::deque<std::string> &csv_elements)
{
    try {
        if (!csv_elements.empty()) {
            std::stringstream ss;
            if (csv_elements.size() > 1) {
                // There exists multiple elements within this vector
                for (const auto &csv: csv_elements) {
                    if (&csv == &csv_elements.back()) {
                        // We are at the last element
                        ss << csv << std::endl;
                    } else {
                        ss << csv << ','  << std::endl;
                    }
                }
            } else {
                // There exists only one element within this vector
                ss << csv_elements[0] << std::endl;
            }

            return ss.str();
        }
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(tr("An error has occurred whilst attempting to modify CSV data.\n\n%1").arg(QString::fromStdString(e.what())).toStdString()));
    }

    return std::string();
}

/**
 * @brief StringFuncs::htmlSpecialCharEncoding
 * @author wysota <https://www.qtcentre.org/threads/52456-HTML-Unicode-ampersand-encoding?p=234858#post234858>,
 * Phobos A. D'thorga <phobos.gekko@gekkofyre.io>.
 * @param string
 * @return
 */
QString StringFuncs::htmlSpecialCharEncoding(const QString &string)
{
    QString encoded;
    for (auto ch: string) {
        if (ch.unicode() > 255) {
            encoded += QString("&#%1;").arg((int)ch.unicode());
        } else {
            encoded += ch;
        }
    }

    return encoded;
}

/**
 * @brief StringFuncs::extractNumbersFromStr will extract any identifiable integers from a given QString.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param str
 * @return
 */
QList<int> StringFuncs::extractNumbersFromStr(const QString &str)
{
    try {
        QList<int> ret_val;
        QRegularExpression rx("d(\\d+)");
        QRegularExpressionMatchIterator i = rx.globalMatch(str);
        while (i.hasNext()) {
            QRegularExpressionMatch match = i.next();
            QString integer = match.captured(1);
            ret_val << integer.toInt();
        }

        return ret_val;
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));
    }

    return QList<int>();
}

/**
 * @brief StringFuncs::zeroPadding
 * @author Tianfu Information <https://www.tfzx.net/en/article/2882643.html>,
 * Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param num
 * @param places
 * @return
 */
QString StringFuncs::zeroPadding(const QVariant &num, const qint32 &places)
{
    QString conv_num = num.toString().toUpper();
    return conv_num.length() < places ? zeroPadding("0" + conv_num, places) : conv_num;
}

/**
 * @brief StringFuncs::fileSizeHumanReadable will 'prettify' a given file-size, converting it to kilobytes, megabytes,
 * gigabytes, etc. as needed, thusly making it more human readable.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param file_size The size of the file under review, as measured in bytes.
 * @return The cleaned up file-size, outputted as a double.
 */
QString StringFuncs::fileSizeHumanReadable(const qint64 &file_size)
{
    qint64 conv_size = 0.0f;
    QString formatted_val;

    if (file_size >= 0 && file_size <= 1024LL) {
        // Output as bytes!
        conv_size = file_size;
        formatted_val = tr("%1 bytes").arg(QString::number(conv_size));
    } else if (file_size >= 1025 && file_size <= (1024LL * 1024LL)) {
        // Output as kilobytes!
        conv_size = (file_size / (1024LL));
        formatted_val = tr("%1 kB").arg(QString::number(conv_size));
    } else if (file_size >= ((1024LL * 1024LL) + 1) && file_size <= (1024LL * 1024LL * 1024LL)) {
        // Output as megabytes!
        conv_size = (file_size / (1024LL * 1024LL));
        formatted_val = tr("%1 MiB").arg(QString::number(conv_size));
    } else if (file_size >= ((1024LL * 1024LL * 1024LL) + 1) && file_size <= (1024LL * 1024LL * 1024LL * 1024LL)) {
        // Output as gigabytes!
        conv_size = (file_size / (1024LL * 1024LL * 1024LL));
        formatted_val = tr("%1 GiB").arg(QString::number(conv_size));
    } else {
        // Output as terabytes!
        conv_size = (file_size / (1024LL * 1024LL * 1024LL * 1024LL));
        formatted_val = tr("%1 TiB").arg(QString::number(conv_size));
    }

    return formatted_val;
}

/**
 * @brief StringFuncs::getPeakValue
 * @author thibsc <https://stackoverflow.com/questions/50277132/qt-audio-file-to-wave-like-audacity>.
 * @param format
 * @return
 * @note The byte-sample, 24-bits, is not supported!
 */
qreal StringFuncs::getPeakValue(const QAudioFormat &format)
{
    qreal ret(0);
    if (format.isValid()) {
        switch (format.sampleType()) {
            case QAudioFormat::Unknown:
                break;
            case QAudioFormat::Float:
                if (format.sampleSize() != 32) { // Other sample formats are not supported!
                    ret = 0;
                } else {
                    ret = 1.00003;
                }

                break;
            case QAudioFormat::SignedInt:
                if (format.sampleSize() == 32) {
                    #ifdef Q_OS_WIN
                    ret = INT_MAX;
                    #endif
                    #ifdef Q_OS_UNIX
                    ret = SHRT_MAX;
                    #endif
                } else if (format.sampleSize() == 16) {
                    ret = SHRT_MAX;
                } else if (format.sampleSize() == 8) {
                    ret = CHAR_MAX;
                }

                break;
            case QAudioFormat::UnSignedInt:
                if (format.sampleSize() == 32) {
                    ret = UINT_MAX;
                } else if (format.sampleSize() == 16) {
                    ret = USHRT_MAX;
                } else if (format.sampleSize() == 8) {
                    ret = UCHAR_MAX;
                }

                break;
            default:
                break;
        }
    }

    return ret;
}

/**
 * @brief StringFuncs::convSecondsToMinutes is a helper function that converts seconds to minutes, provided that the given seconds
 * are longer than a single minute in length, otherwise the value remains as seconds for ease-of-reading.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param seconds The seconds to be converted to minutes.
 * @return A pretty-string is outputted of the converted seconds, along with the time value at the end (i.e. seconds, minutes,
 * hours, etc.) so that the user knows the actual length of time in question.
 */
QString StringFuncs::convSecondsToMinutes(const double &seconds) {
    double conv_val = seconds;
    std::stringstream ss;
    QString time_value;
    if (seconds > 0.0f && seconds < 60.0f) {
        conv_val = seconds;
        time_value = tr("seconds");
    } else if (seconds > 60.0f) {
        conv_val /= 60.0f;
        time_value = tr("minutes");
    } else if (seconds > (60.0f * 60.0f)) {
        conv_val /= (60.0f * 60.0f);
        time_value = tr("hours");
    } else if (seconds > (60.0f * 60.0f * 24.0f)) {
        conv_val /= (60.0f * 60.0f * 24.0f);
        time_value = tr("days");
    } else {
        conv_val /= (60.0f * 60.0f * 24.0f * 7.0f);
        time_value = tr("weeks");
    }

    ss << conv_val << " " << time_value.toStdString();

    return QString::fromStdString(ss.str());
}

/**
 * @brief StringFuncs::randomNumGen creates a randomly generated number from the given lower and upper bounds.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param lower_bound The lowest given input number to use.
 * @param upper_bound The maximum given input number to use.
 * @return The randomly generated number from the given upper and lower bounds.
 */
qint32 StringFuncs::randomNumGen(qint32 lower_bound, qint32 upper_bound)
{
    const qint32 lb = lower_bound;
    const qint32 ub = upper_bound;

    static std::random_device rand_dev;
    std::mt19937 mt(rand_dev());
    std::uniform_int_distribution<int> distrib(lb, ub);
    return distrib(mt);
}

/**
 * @brief StringFuncs::changePushButtonColor
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param push_button The QPushButton to be modified with the new QStyleSheet.
 * @param red_result Whether to make the QPushButton in question Green or Red.
 * @param color_blind_mode Not yet implemented!
 */
void StringFuncs::changePushButtonColor(const QPointer<QPushButton> &push_button, const bool &red_result,
                                        const bool &color_blind_mode)
{
    Q_UNUSED(color_blind_mode);

    if (red_result) {
        // Change QPushButton to a shade of darkish 'Green'
        push_button->setStyleSheet("QPushButton{\nbackground-color: #B80000; border: 1px solid black;\nborder-radius: 5px;\nborder-width: 1px;\npadding: 6px;\nfont: bold;\ncolor: white;\n}");
    } else {
        // Change QPushButton to a shade of darkish 'Red'
        push_button->setStyleSheet("QPushButton{\nbackground-color: #3C8C2F; border: 1px solid black;\nborder-radius: 5px;\nborder-width: 1px;\npadding: 6px;\nfont: bold;\ncolor: white;\n}");
    }

    // TODO: Implement color-blind mode!
    return;
}

#if defined(_WIN32) || defined(__MINGW64__) || defined(__CYGWIN__)
/**
 * @brief StringFuncs::convQStringToWinBStr converts a given QString to a Microsoft Windows compatible Binary
 * String (i.e. `BSTR` variable).
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
BSTR StringFuncs::convQStringToWinBStr(const QString &str_to_convert)
{
    BSTR result = SysAllocStringLen(0, str_to_convert.length());
    str_to_convert.toWCharArray(result);
    return result;
}

/**
 * @brief StringFuncs::convWinBstrToQString converts a given Microsoft Windows compatible Binary String to a QString.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
QString StringFuncs::convWinBstrToQString(const BSTR &str_to_convert)
{
    QString result = QString::fromUtf16(reinterpret_cast<quint16 *>(str_to_convert));
    return result;
}

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
#endif
