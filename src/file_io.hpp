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

#include "defines.hpp"
#include <QObject>
#include <boost/filesystem.hpp>
#include <boost/exception/all.hpp>
#include <functional>
#include <vector>
#include <string>

namespace GekkoFyre {

class FileIo : public QObject {
    Q_OBJECT

public:
    explicit FileIo(QObject *parent = nullptr);
    ~FileIo() override;

    static std::vector<boost::filesystem::path> boost_dir_iterator(const boost::filesystem::path &dirPath, boost::system::error_code ec,
            const std::vector<std::string> &dirsToSkip = { });
    static bool checkSettingsExist(const bool &is_file, const boost::filesystem::path &fileName = GekkoFyre::Filesystem::fileName);

    static std::string create_random_string(size_t length);
    boost::filesystem::path dummy_path();

protected:
    static std::vector<boost::filesystem::path> analyze_dir(const boost::filesystem::path &dirPath, const std::vector<std::string> &dirsToSkip = { });

private:
    static std::string get_file_contents(const boost::filesystem::path &filePath);
    static std::string init_random_string(size_t length, std::function<char(void)> rand_char);
    static char_array charset();

};
};
