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

#include "src/update/gk_check_update.hpp"
#include <iostream>

using namespace GekkoFyre;

GkCheckUpdate::GkCheckUpdate(QObject *parent) : QObject(parent)
{
    aria2::SessionConfig config;
    aria_session.reset(aria2::sessionNew(aria2::KeyVals(), config), [](aria2::Session* s){aria2::sessionFinal(s);});

    return;
}

GkCheckUpdate::~GkCheckUpdate()
{}

/**
 * @brief GkCheckUpdate::downloadEventCallback
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param session
 * @param event
 * @param gid
 * @param userData
 * @return
 */
qint32 GkCheckUpdate::downloadEventCallback(std::shared_ptr<aria2::Session> session, aria2::DownloadEvent event,
                                            aria2::A2Gid gid, void *userData)
{
    switch (event) {
        case aria2::EVENT_ON_DOWNLOAD_COMPLETE:
            break;
        case aria2::EVENT_ON_DOWNLOAD_ERROR:
            break;
        default:
            return 0;
    }

    std::cerr << " [" << aria2::gidToHex(gid) << "] ";
    aria2::DownloadHandle* dh = aria2::getDownloadHandle(session.get(), gid);
    if (!dh) {
        return 0;
    }

    if (dh->getNumFiles() > 0) {
        aria2::FileData f = dh->getFile(1);

        //
        // Path maybe empty if the filename has not been determined yet!
        if (f.path.empty()) {
            if (!f.uris.empty()) {
                std::cerr << f.uris[0].uri;
            }
        } else {
            std::cerr << f.path;
        }
    }

    aria2::deleteDownloadHandle(dh);
    std::cerr << std::endl;

    return 0;
}
