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
 **   Copyright (C) 2020 - 2022. GekkoFyre.
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

namespace GekkoFyre {

class FileIo : public QObject {
    Q_OBJECT

public:
    explicit FileIo(QObject *parent = nullptr);
    ~FileIo() override;

    static std::vector<boost::filesystem::path> boost_dir_iterator(const boost::filesystem::path &dirPath, boost::system::error_code ec,
            const std::vector<std::string> &dirsToSkip = { });
    static bool checkSettingsExist(const bool &is_file, const boost::filesystem::path &fileName = GekkoFyre::Filesystem::fileName);

    void write_initial_settings(const QString &value, const GekkoFyre::Database::Settings::init_cfg &key);
    QString read_initial_settings(const GekkoFyre::Database::Settings::init_cfg &key);

    [[nodiscard]] size_t generateRandInteger(const size_t &min_integer_size, const size_t &max_integer_size,
                               const size_t &desired_result_less_than) const;

    std::string get_file_contents(const boost::filesystem::path &filePath);
    QString defaultDirectory(const QString &base_path, const bool &use_native_slashes = false,
                             const QString &append_dir = QString(General::companyName + QString("/") + Filesystem::defaultDirAppend));

protected:
    static std::vector<boost::filesystem::path> analyze_dir(const boost::filesystem::path &dirPath, const std::vector<std::string> &dirsToSkip = { });

};
};
