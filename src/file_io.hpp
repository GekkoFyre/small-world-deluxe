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
#include <boost/filesystem.hpp>
#include <boost/exception/all.hpp>
#include <boost/random.hpp>
#include <boost/random/random_device.hpp>
#include <functional>
#include <algorithm>
#include <iterator>
#include <vector>
#include <random>
#include <string>
#include <memory>
#include <limits>
#include <array>
#include <QObject>
#include <QString>
#include <QSettings>

namespace GekkoFyre {

class FileIo : public QObject {
    Q_OBJECT

public:
    explicit FileIo(std::shared_ptr<QSettings> settings, QObject *parent = nullptr);
    ~FileIo() override;

    static std::vector<boost::filesystem::path> boost_dir_iterator(const boost::filesystem::path &dirPath, boost::system::error_code ec,
            const std::vector<std::string> &dirsToSkip = { });
    static bool checkSettingsExist(const bool &is_file, const boost::filesystem::path &fileName = GekkoFyre::Filesystem::fileName);

    void write_initial_settings(const QString &value, const GekkoFyre::Database::Settings::init_cfg &key);
    QString read_initial_settings(const GekkoFyre::Database::Settings::init_cfg &key);

    size_t generateRandInteger(const size_t &min_integer_size, const size_t &max_integer_size,
                               const size_t &desired_result_less_than) const;
    static std::string create_random_string(const size_t &len);
    boost::filesystem::path dummy_path();

    std::string get_file_contents(const boost::filesystem::path &filePath);
    QString defaultDirectory(const QString &base_path, const bool &use_native_slashes = false,
                             const QString &append_dir = Filesystem::defaultDirAppend);

protected:
    static std::vector<boost::filesystem::path> analyze_dir(const boost::filesystem::path &dirPath, const std::vector<std::string> &dirsToSkip = { });

private:
    std::shared_ptr<QSettings> gkSettings;

    //
    // Author: Konrad Rudolph <https://stackoverflow.com/a/444614/4293625>
    //
    template <typename T = boost::mt19937>
    static auto random_generator() -> T {
        auto constexpr seed_bits = sizeof(typename T::result_type) * T::state_size;
        auto constexpr seed_len = seed_bits / std::numeric_limits<std::seed_seq::result_type>::digits;
        auto seed = std::array<std::seed_seq::result_type, seed_len>{};
        auto dev = boost::random_device{};
        std::generate_n(begin(seed), seed_len, std::ref(dev));
        auto seed_seq = std::seed_seq(std::begin(seed), std::end(seed));
        return T{seed_seq};
    }
};
};
