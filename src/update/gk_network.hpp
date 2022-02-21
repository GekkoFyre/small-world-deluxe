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

#include <aria2/aria2.h>
#include <mutex>
#include <vector>
#include <memory>
#include <string>
#include <thread>
#include <QQueue>
#include <QObject>
#include <QString>
#include <QIODevice>

namespace GekkoFyre {
    namespace Network {
        enum GkDataState {
            Paused,
            Downloading,
            Uploading
        };

        struct GkDownload {
            GkDataState state;
            std::string uri;
            std::shared_ptr<aria2::DownloadHandle> handle;
        };
    }

class GkNetwork : public QObject {
    Q_OBJECT

public:
    explicit GkNetwork(QObject *parent = nullptr);
    ~GkNetwork() override;

public slots:
    void modifyNetworkState(const Network::GkDataState &network_state);

signals:
    void recvFileFromUrl(const std::string &recv_url);
    void changeNetworkState(const Network::GkDataState &network_state);
    void startDownload(const std::vector<QString> &uris);
    void pauseDownload(const std::vector<QString> &uris);
    void stopDownload(const std::vector<QString> &uris);
    void sendDownloadHandle(const std::shared_ptr<aria2::DownloadHandle> &dh);

private slots:
    void addFileToDownload(const std::string &recv_url);
    void dataCaptureFromUri(const std::vector<QString> &recv_uris);
    void suspendDownload(const std::vector<QString> &uris);
    void removeDownload(const std::vector<QString> &uris);

private:
    //
    // Aria2-related variables, etc.
    Network::GkDataState m_networkState;
    std::shared_ptr<aria2::Session> aria_session;
    QQueue<Network::GkDownload> dlQueue;

    static qint32 downloadEventCallback(aria2::Session *session, aria2::DownloadEvent event, aria2::A2Gid gid,
                                        void *userData);
    void processDownloads();

    std::string convTo8BitStr(const QString &str_to_conv);
    void print_exception(const std::exception &e, int level = 0);

};
};
