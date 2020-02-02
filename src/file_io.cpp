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
 **   [ 1 ] - https://git.gekkofyre.io/amateur-radio/small-world-deluxe
 **
 ****************************************************************************************************/

#include "file_io.hpp"
#include <boost/range/iterator_range.hpp>
#include <exception>
#include <algorithm>
#include <random>
#include <iostream>
#include <cstring>
#include <QMessageBox>
#include <QString>
#include <utility>

using namespace GekkoFyre;
namespace fs = boost::filesystem;
namespace sys = boost::system;

/**
 * @brief FileIo::FileIo Primarily responsible for the archiving of the user's RocksDB database files, which are generated by this program. These store user-defined settings.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param parent N/A.
 */
FileIo::FileIo(QObject *parent)
{
    Q_UNUSED(parent);
}

FileIo::~FileIo()
= default;

/**
 * @brief FileIo::create_random_string Creates a random string of given length
 * @author https://stackoverflow.com/users/13760/carl
 * @param length The given length of the random string
 * @see GekkoFyre::FileIo::init_random_string()
 * @return The generated random string.
 */
std::string FileIo::create_random_string(size_t length)
{
    const auto ch_set = charset();
    std::default_random_engine rng(std::random_device{}());
    std::uniform_int_distribution<> dist(0, ch_set.size()-1);
    auto randchar = [ ch_set, &dist, &rng ](){return ch_set[ dist(rng) ];};
    auto rand_string = init_random_string(length, std::function<char(void)>(randchar));
    return rand_string;
}

/**
 * @brief FileIo::random_string Writes out a random string of any given length.
 * @author Carl <https://stackoverflow.com/questions/440133/how-do-i-create-a-random-alpha-numeric-string-in-c>
 * @param length The given length of the random string to be generated.
 * @param rand_char Characters to optionally use.
 * @see GekkoFyre::FileIo::create_random_string()
 * @return The generated random string.
 */
std::string FileIo::init_random_string(size_t length, std::function<char(void)> rand_char)
{
    std::string str(length, 0);
    std::generate_n(str.begin(), length, std::move(rand_char));
    return str;
}

/**
 * @brief FileIo::dummy_path Creates a dummy pathname to a temporary file for the purposes of resolving errors
 * and other edge-cases in a more clean manner.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @return The full pathname to a dummy file within a declared temporary directory for the current user.
 */
boost::filesystem::path FileIo::dummy_path()
{
    std::string rand_file_name = create_random_string(8);
    fs::path slash = "/";
    fs::path native_slash = slash.make_preferred().native();
    const fs::path temp_file_path = fs::path(fs::temp_directory_path().string() + native_slash.string() + rand_file_name + ".tmp");
    return temp_file_path;
}

/**
 * @brief FileIo::analyze_dir Will go through a directory and add all the files/dirs within
 * to a typical std::vector, ready for use.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param dirPath The full-path to where the directory in question is located.
 * @see GekkoFyre::FileIo::pack_archive()
 * @return The analyzed directories and files, in a typical std::vector format.
 */
std::vector<fs::path> FileIo::analyze_dir(const boost::filesystem::path &dirPath, const std::vector<std::string> &dirsToSkip)
{
    sys::error_code err;
    if (fs::exists(dirPath, err)) {
        if (fs::is_directory(dirPath, err)) {
            std::vector<fs::path> dir_list = boost_dir_iterator(dirPath, err, dirsToSkip);
            if (err) {
                throw std::runtime_error(err.message());
            }

            if (!dir_list.empty()) {
                std::cout << tr("Successfully analyzed directories and files.").toStdString() << std::endl;
                return dir_list;
            }
        }
    }

    std::cerr << tr("Issue encountered with analyzing directories and files! Error: %1")
                 .arg(QString::fromStdString(err.message())).toStdString() << std::endl;

    return std::vector<fs::path>();
}

/**
 * @brief FileIo::checkSettingsExist Checks to see if the Google LevelDB settings database exists on the user's storage or not.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param fileName If provided, it will check for the custom filename at the pre-determined destination instead.
 * @return Whether the settings database on the user's storage exists or not.
 */
bool FileIo::checkSettingsExist(const bool &is_file, const boost::filesystem::path &fileName)
{
    sys::error_code ec;
    try {
        fs::path slash = "/";
        fs::path native_slash = slash.make_preferred().native();

        // Path to save final database towards
        fs::path save_db_path = fs::path(fs::current_path().string() + native_slash.string() + fileName.string());
        if (fs::exists(save_db_path, ec)) {
            if (is_file) {
                if (fs::is_regular_file(save_db_path, ec)) {
                    return true;
                } else {
                    throw ec.message();
                }
            } else {
                if (fs::is_directory(save_db_path, ec)) {
                    return true;
                } else {
                    throw ec.message();
                }
            }
        } else {
            throw ec.message();
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), e.what(), QMessageBox::Ok);
    }

    return false;
}

/**
 * @brief FileIo::get_file_contents Grabs the contents of a file and adds it to a std::string().
 * @author Originally authored by the Google Snappy team: https://github.com/google/snappy
 * @param filePath The path of the file to be grabbed.
 * @return Whether The data contents within of the grabbed file in question.
 */
std::string FileIo::get_file_contents(const boost::filesystem::path &filePath)
{
    try {
        if (fs::is_regular_file(filePath)) {
            FILE* fp = fopen(filePath.string().c_str(), "rb");
            if (fp == nullptr) {
                throw std::invalid_argument(tr("Error encountered with opening filestream: %1\n\nAborting...").arg(QString::fromStdString(filePath.string())).toStdString());
            }

            std::fseek(fp, 0, SEEK_END);
            long long int len = std::ftell(fp);
            if (len < 0) {
                throw std::invalid_argument(tr("Error encountered with reading filestream: %1\n\nAborting...").arg(QString::fromStdString(filePath.string())).toStdString());
            }

            std::fseek(fp, 0, SEEK_SET);
            std::string contents(len + 1, '\n'); // https://github.com/facebook/leveldb/blob/master/db/version_set.cc [ CURRENT file does not end with newline ]
            std::fread(&contents[0], 1, len, fp);

            fclose(fp);
            return contents;
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), e.what(), QMessageBox::Ok);
    }

    return "";
}

char_array FileIo::charset()
{
    return char_array(
    {'0','1','2','3','4',
    '5','6','7','8','9',
    'A','B','C','D','E','F',
    'G','H','I','J','K',
    'L','M','N','O','P',
    'Q','R','S','T','U',
    'V','W','X','Y','Z',
    'a','b','c','d','e','f',
    'g','h','i','j','k',
    'l','m','n','o','p',
    'q','r','s','t','u',
    'v','w','x','y','z'
    });
}

/**
 * @brief FileIo::boost_dir_iterator Will recursively scan a directory for its contents such as files, folders and symlinks
 * before outputting the discoveries as a std::vector containing Boost C++ Filesystem path structures.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param dirPath The target directory to iteratively scan.
 * @note https://stackoverflow.com/questions/20923456/boost-directory-iterator-example-how-to-list-directory-files-not-recursive
 * @return The discovered contents of the targeted directory, whether they be folders, files, symlinks, etc.
 */
std::vector<boost::filesystem::path> FileIo::boost_dir_iterator(const boost::filesystem::path &dirPath, sys::error_code ec,
        const std::vector<std::string> &dirsToSkip)
{
    try {
        if (fs::exists(dirPath, ec)) {
            if (fs::is_directory(dirPath, ec)) {
                std::vector<boost::filesystem::path> file_list;

                // Create recursive directory iterators that point towards the starting and ending of a directory
                fs::recursive_directory_iterator iter(dirPath);
                fs::recursive_directory_iterator end;

                while (iter != end) {
                    if (fs::is_directory(iter->path(), ec) && (std::find(dirsToSkip.begin(), dirsToSkip.end(), iter->path().filename()) != dirsToSkip.end())) {
                        iter.no_push();
                    } else {
                        file_list.emplace_back(iter->path().string());
                    }

                    iter.increment(ec); // Increment towards the next entry

                    if (ec) {
                        std::ostringstream error_msg;
                        error_msg << iter->path().string() << "::" << ec.message();

                        std::cerr << tr("Error whilst accessing path: %1").arg(QString::fromStdString(error_msg.str())).toStdString() << std::endl;
                        throw std::runtime_error(tr("Error whilst accessing path: %1").arg(QString::fromStdString(error_msg.str())).toStdString());
                    }
                }

                return file_list;
            }
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), e.what(), QMessageBox::Ok);
    }

    return std::vector<boost::filesystem::path>();
}
