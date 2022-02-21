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

#include "src/update/gk_network.hpp"
#include <future>
#include <utility>
#include <iostream>
#include <exception>
#include <QBuffer>
#include <QSaveFile>
#include <QByteArray>
#include <QMessageBox>

using namespace GekkoFyre;
using namespace Network;

/**
 * @brief GkNetwork::GkNetwork
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param parent
 */
GkNetwork::GkNetwork(QObject *parent) : QObject(parent)
{
    //
    // Setup and initiate Aria2!
    aria2::libraryInit();
    aria2::SessionConfig config;
    config.downloadEventCallback = downloadEventCallback;
    aria_session.reset(aria2::sessionNew(aria2::KeyVals(), config), [](aria2::Session* s){aria2::sessionFinal(s);});

    //
    // Setup any SIGNALs and SLOTs!
    QObject::connect(this, SIGNAL(changeNetworkState(const Network::GkXmpp::GkDataState &)),
                     this, SLOT(modifyNetworkState(const Network::GkXmpp::GkDataState &)));
    QObject::connect(this, SIGNAL(recvFileFromUrl(const QString &)),
                     this, SLOT(recvFileViaHttp(const QString &)));
    QObject::connect(this, SIGNAL(startDownload(const std::vector<std::string> &)),
                     this, SLOT(dataCaptureFromUri(const std::vector<std::string> &)));

    return;
}

GkNetwork::~GkNetwork()
{
    return;
}

/**
 * @brief GkNetwork::recvFileViaHttp will attempt to receive/download a file, remotely, via the HTTP(S) protocol over
 * the Internet (i.e. a wide area network).
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param recv_url The URL for which to attempt to download as a file.
 * @note C++ library interface to aria2 <http://aria2.github.io/manual/en/html/libaria2.html>.
 */
void GkNetwork::recvFileViaHttp(const QString &recv_url)
{
    //
    // Added download item(s) to the aforementioned session which has just been created!
    qint32 rv;
    for (qint32 i = 1; i < recv_url; ++i) {
        //
        // Add to the queue, pre-existing or not!
        uris.emplace(convTo8BitStr(recv_url));
    }

    return;
}

/**
 * @brief GkNetwork::dataCaptureFromUri initializes/realizes the download process from a given URI or URIs, capturing
 * the data from a remote server over either a WAN or a LAN.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param recv_uris The URI or URIs in which to attempt to download as a file or set of files.
 */
void GkNetwork::dataCaptureFromUri(const std::vector<std::string> &recv_uris)
{
    qint32 rv;
    auto start = std::chrono::steady_clock::now();
    for (;;) {
        rv = aria2::run(aria_session.get(), aria2::RUN_ONCE);
        if (rv != 1) {
            break;
        }

        const auto now = std::chrono::steady_clock::now();
        const auto count = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();

        //
        // Print progress information per every 500 milliseconds!
        if (count >= 500) {
            start = now;
            aria2::GlobalStat gstat = aria2::getGlobalStat(aria_session.get());
            std::vector<aria2::A2Gid> gids = aria2::getActiveDownload(aria_session.get());
        }
    }

    return;
}

/**
 * @brief GkNetwork::modifyNetworkState signifies the current state for this class, of whether we are actively uploading,
 * downloading, or have paused either process with regard to data to/from the Internet.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param network_state The current network state for this class, with regard to how data is being handled.
 * @note C++ library interface to aria2 <http://aria2.github.io/manual/en/html/libaria2.html>.
 */
void GkNetwork::modifyNetworkState(const GkDataState &network_state)
{
    try {
        m_networkState = network_state;
        switch (m_networkState) {
            case GkDataState::Paused:
                break;
            case GkDataState::Downloading:
                processDownloads();

                break;
            case GkDataState::Uploading:
                break;
            default:
                throw std::invalid_argument(tr("Invalid network state has been given for download/upload engine!").toStdString());
        }
    } catch (const std::exception &e) {
        print_exception(e);
    }

    return;
}

/**
 * @brief GkNetwork::downloadEventCallback
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param session
 * @param event
 * @param gid
 * @param userData
 * @return
 * @note C++ library interface to aria2 <http://aria2.github.io/manual/en/html/libaria2.html>,
 * libaria2ex.cc <https://github.com/aria2/aria2/blob/master/examples/libaria2ex.cc>.
 */
qint32 GkNetwork::downloadEventCallback(aria2::Session *session, aria2::DownloadEvent event, aria2::A2Gid gid,
                                        void *userData)
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
    aria2::DownloadHandle* dh = aria2::getDownloadHandle(session, gid);
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

/**
 * @brief GkNetwork::processDownloads
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkNetwork::processDownloads()
{
    try {
        qint32 rv;
        std::vector<std::string> uri_vec_tmp;

        while (!uris.empty()) {
            const std::string conv_uri_str = uris.front();
            uri_vec_tmp.emplace_back(conv_uri_str);
            aria2::KeyVals options;
            rv = aria2::addUri(aria_session.get(), nullptr, uri_vec_tmp, options);
            if (rv < 0) {
                throw std::invalid_argument(tr("Failed to add download to the queue! URI in question: \"%1\"")
                .arg(QString::fromStdString(conv_uri_str)).toStdString());
            }
        }
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error(e.what()));;
    }

    return;
}

/**
 * @brief GkNetwork::convTo8BitStr
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param str_to_conv
 * @return
 */
std::string GkNetwork::convTo8BitStr(const QString &str_to_conv)
{
    if (!str_to_conv.isEmpty() && !str_to_conv.isNull()) {
        std::string conv_txt; // For equality comparison purposes!
        #if defined(_WIN32) || defined(__MINGW64__) || defined(__CYGWIN__)
        conv_txt = str_to_conv.toUtf8().constData();
        #else
        conv_txt = str_to_conv.toLocal8Bit().constData();
        #endif

        return conv_txt;
    }

    return std::string();
}

/**
 * @brief GkNetwork::print_exception
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param e
 * @param level
 */
void GkNetwork::print_exception(const std::exception &e, int level)
{
    QMessageBox::critical(nullptr, tr("Error!"), e.what(), QMessageBox::Ok);

    try {
        std::rethrow_if_nested(e);
    } catch (const std::exception &e) {
        print_exception(e, level + 1);
    } catch (...) {}

    return;
}
