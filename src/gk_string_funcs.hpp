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
#include <boost/filesystem.hpp>
#include <QAudioFormat>
#include <QMessageBox>
#include <QPushButton>
#include <QPointer>
#include <QObject>
#include <QString>
#include <QList>
#include <string>
#include <memory>
#include <vector>
#include <limits>
#include <mutex>
#include <deque>

#ifdef _WIN32
#include <Windows.h>
#include <oleauto.h>
#endif

namespace GekkoFyre {

class StringFuncs : public QObject {
    Q_OBJECT

public:
    explicit StringFuncs(QObject *parent = nullptr);
    ~StringFuncs() override;

    #if defined(_WIN32) || defined(__MINGW64__)
    BSTR convQStringToWinBStr(const QString &str_to_convert);
    QString convWinBstrToQString(const BSTR &str_to_convert);
    static std::string multiByteFromWide(LPCWSTR pwsz, UINT cp);
    static std::wstring strToWStrWin(const std::string &s);
    std::wstring removeSpecialChars(std::wstring wstr);
    bool modalDlgBoxOk(const HWND &hwnd, const QString &title, const QString &msgTxt, const int &icon);
    #endif

    void print_exception(const std::exception &e, int level = 0);

    QString getStringFromUnsignedChar(unsigned char *str);
    std::vector<int> convStrToIntArray(const QString &str);
    QString addErrorMsg(const QString &orig_msg, const QString &err_msg);

    std::deque<std::string> csvSplitter(const std::string &csv_vals);
    std::deque<std::string> csvRemoveElement(const std::deque<std::string> &csv_elements, const std::string &val_to_remove);
    std::string csvOutputString(const std::deque<std::string> &csv_elements);

    QList<int> extractNumbersFromStr(const QString &str);
    QString zeroPadding(const QVariant &num, const qint32 &places);

    QString fileSizeHumanReadable(const qint64 &file_size);
    qreal getPeakValue(const QAudioFormat &format);

    QString convSecondsToMinutes(const double &seconds);
    qint32 randomNumGen(qint32 lower_bound = std::numeric_limits<qint32>::min(), qint32 upper_bound = std::numeric_limits<qint32>::max());
    void changePushButtonColor(const QPointer<QPushButton> &push_button, const bool &red_result = true,
                               const bool &color_blind_mode = false);

        template<typename T>
        T getNumericMax() {
            return std::numeric_limits<T>::max();
        }

        template<typename T>
        T getNumericMin() {
            return std::numeric_limits<T>::min();
        }

    /**
     * @brief StringFuncs::splitVec will split a given std::vector<T> into many sub-vectors of a given size. This is
     * particularly useful for multithreading, for example, as it could split the vector into a desired number of chunks
     * according to how many CPUs/cores the end-user has.
     * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
     * @tparam T The desired datatype.
     * @param input_data The input data you wish to have split-up.
     * @param desired_size The (approximate) resultant size of your given sub-vectors.
     * @return The desired sub-vectors, as according to the given (approximate) size.
     */
        template<typename T>
        std::vector<std::vector<T>> splitVec(const std::vector<T> &input_data, const size_t &desired_size) {
            std::vector<std::vector<T>> ret_vec;
            size_t length = input_data.size() / desired_size;
            size_t remaining = input_data.size() % desired_size;

            size_t begin = 0;
            size_t end = 0;

            for (size_t i = 0; i < std::min(desired_size, input_data.size()); ++i) {
                end += (remaining > 0) ? (length + !!(remaining--)) : length;
                ret_vec.emplace_back(std::vector<T>(input_data.begin() + begin, input_data.begin() + end));
                begin = end;
            }

            return ret_vec;
        }

    /**
     * @brief StringFuncs::chunker will split a given std::vector<T> into many sub-vectors of a given size. This is
     * particularly useful for multithreading, for example, if you need multiple std::vector's of a particular size
     * each.
     * @author marcinwol <https://gist.github.com/marcinwol/3283a92331ff64a8f531>.
     * @return The desired sub-vectors, as according to the given (approximate) size.
     */
    template <typename T, typename A, template <typename , typename > class C>C<C<T,A>, std::allocator<C<T,A>>>
    chunker(C<T,A> &c, const typename C<T,A>::size_type &k) {
        if (k <= 0) {
            throw std::domain_error(tr("chunker() requires k > 0").toStdString());
        }

        std::lock_guard<std::mutex> lock_g(chunker_mtx);
        using INPUT_CONTAINER_TYPE = C<T,A>;
        using INPUT_CONTAINER_VALUE_TYPE = typename INPUT_CONTAINER_TYPE::value_type;
        using OUTPUT_CONTAINER_TYPE = C<INPUT_CONTAINER_TYPE, std::allocator<INPUT_CONTAINER_TYPE>>;

        OUTPUT_CONTAINER_TYPE out_c;

        auto chunkBeg = begin(c);
        for (auto left = c.size(); left != 0;) {
            auto const skip = std::min(left, k);
            INPUT_CONTAINER_TYPE sub_container;
            std::back_insert_iterator<INPUT_CONTAINER_TYPE> back_v(sub_container);
            std::copy_n(chunkBeg, skip, back_v);
            out_c.push_back(sub_container);
            left -= skip;
            std::advance(chunkBeg, skip);
        }

        return out_c;
    }

private:
    std::mutex chunker_mtx;

};
};
