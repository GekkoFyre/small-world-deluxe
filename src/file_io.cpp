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

#include "src/file_io.hpp"
#include <boost/range/iterator_range.hpp>
#include <exception>
#include <iostream>
#include <cctype>
#include <ostream>
#include <utility>
#include <fstream>
#include <QDir>
#include <QMessageBox>
#include <QVariant>
#include <QStandardPaths>

using namespace GekkoFyre;
using namespace Database;
using namespace Settings;
using namespace Language;
using namespace Audio;
using namespace AmateurRadio;
using namespace Control;
using namespace Spectrograph;
using namespace System;
using namespace Events;
using namespace Logging;

namespace fs = boost::filesystem;
namespace sys = boost::system;

/**
 * @brief FileIo::FileIo Primarily responsible for the archiving of the user's RocksDB database files, which are generated by this program. These store user-defined settings.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param parent N/A.
 */
FileIo::FileIo(QObject *parent)
{
    return;
}

FileIo::~FileIo()
{
    return;
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
    try {
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
    } catch (const sys::system_error &e) {
        err = e.code();
        QMessageBox::warning(nullptr, tr("Error!"), tr("Issue encountered with analyzing directories and files! Error:\n\n%1").arg(err.message().c_str()),
                             QMessageBox::Ok);
    }

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
    } catch (const sys::system_error &e) {
        ec = e.code();
        QMessageBox::warning(nullptr, tr("Error!"), ec.message().c_str(), QMessageBox::Ok);
    }

    return false;
}

/**
 * @brief FileIo::write_initial_settings
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param value
 * @param key
 */
void FileIo::write_initial_settings(const QString &value, const Database::Settings::init_cfg &key)
{
    /*
    switch (key) {
    case DbName:
        gkSettings->setValue(Settings::dbName, value);
        break;
    case DbExt:
        gkSettings->setValue(Settings::dbExt, value);
        break;
    case DbLoc:
        gkSettings->setValue(Settings::dbLoc, value);
        break;
    default:
        return;
    }
    */

    return;
}

/**
 * @brief FileIo::read_initial_settings
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param key
 * @return
 */
QString FileIo::read_initial_settings(const Database::Settings::init_cfg &key)
{
    QVariant value;

    fs::path slash = "/";
    fs::path native_slash = slash.make_preferred().native();

    switch (key) {
    case DbName:
        value = QString::fromStdString(fs::path(General::companyName + native_slash.string() + Filesystem::defaultDirAppend +
                                        native_slash.string() + Filesystem::fileName).string());
        break;
    case DbExt:
        value = "";
        break;
    case DbLoc:
        value = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
        break;
    default:
        return "";
    }

    return value.toString();
}

/**
 * @brief FileIo::get_file_contents Grabs the contents of a file and adds it to a std::string(), in
 * a way that should be portable across operating systems.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param filePath The path of the file to be grabbed.
 * @return Whether The data contents within of the grabbed file in question.
 */
std::string FileIo::get_file_contents(const boost::filesystem::path &filePath)
{
    try {
        if (fs::is_regular_file(filePath)) {
            std::ifstream ifs (filePath.c_str(), std::ios::in | std::ios::binary | std::ios::ate);

            std::ifstream::pos_type file_size = ifs.tellg();
            ifs.seekg(0, std::ios::beg);

            std::vector<char> bytes(file_size);
            ifs.read(bytes.data(), file_size);

            return std::string(bytes.data(), file_size);
        } else {
            throw std::invalid_argument(tr("Invalid file provided!").toStdString());
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(nullptr, tr("Error!"), e.what(), QMessageBox::Ok);
    }

    return "";
}

/**
 * @brief FileIo::defaultDirectory creates a default directory for Small World Deluxe in the
 * specified, given directory as one of the parameters. This is simply a helper function.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param base_path The base path for which to create the new directory.
 * @param use_native_slashes Whether to use native file-path slashes for the given user's
 * operating system or not.
 * @param append_dir The new directory that's to be created.
 * @return The final, full directory path that has been created in the end with the given
 * native slashes or not.
 */
QString FileIo::defaultDirectory(const QString &base_path, const bool &use_native_slashes, const QString &append_dir)
{
    sys::error_code ec;

    try {
        fs::path base_dir = base_path.toStdString();
        fs::path new_dir;
        fs::path slash = "/";
        fs::path native_slash;

        if (use_native_slashes) {
            // Use the 'slashes' that are preferred by the host's operating system...
            native_slash = slash.make_preferred().native();
        } else {
            // Use the programmer's choice...
            native_slash = slash;
        }

        if (!append_dir.isEmpty()) {
            if (fs::exists(base_dir, ec) && fs::is_directory(base_dir, ec)) {
                new_dir = fs::path(base_dir.string() + native_slash.string() + append_dir.toStdString());
                if (!fs::exists(new_dir, ec)) {
                    fs::create_directories((base_dir.string() + native_slash.string() + append_dir.toStdString()), ec);
                }
            }
        }

        return QString::fromStdString(new_dir.string());
    } catch (const sys::system_error &e) {
        ec = e.code();
        QMessageBox::warning(nullptr, tr("Error!"), ec.message().c_str(), QMessageBox::Ok);
    }

    return "";
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
