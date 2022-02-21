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

#include "src/update/ui/gkfiletransferdialog.hpp"
#include "ui_gkfiletransferdialog.h"
#include <QMessageBox>

using namespace GekkoFyre;
using namespace Network;

GkFileTransferDialog::GkFileTransferDialog(QWidget *parent) : QDialog(parent), ui(new Ui::GkFileTransferDialog)
{
    ui->setupUi(this);

    //
    // Setup any pointers!
    gkNetwork = new GkNetwork(parent);

    //
    // Setup SIGNALs and SLOTs!
    QObject::connect(gkNetwork, SIGNAL(sendDownloadHandle(const std::shared_ptr<aria2::DownloadHandle> &)),
                     this, SLOT(recvDownloadHandle(const std::shared_ptr<aria2::DownloadHandle> &)));
    QObject::connect(this, SIGNAL(startDownload(const std::vector<QString> &)),
                     gkNetwork, SIGNAL(startDownload(const std::vector<QString> &)));
    QObject::connect(this, SIGNAL(pauseDownload(const std::vector<QString> &)),
                     gkNetwork, SIGNAL(pauseDownload(const std::vector<QString> &)));
    QObject::connect(this, SIGNAL(stopDownload(const std::vector<QString> &)),
                     gkNetwork, SIGNAL(stopDownload(const std::vector<QString> &)));
}

GkFileTransferDialog::~GkFileTransferDialog()
{
    delete ui;
}

/**
 * @brief GkFileTransferDialog::beginDownload initiates the download process for a file or set of files.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param recv_uris The URI or URIs in which to attempt to download as a file or set of files.
 */
void GkFileTransferDialog::beginDownload(const std::vector<QString> &uris)
{
    std::vector<QString> download_list;
    for (const auto &uri: uris) {
        if (!uri.isNull()) {
            if (!uri.isEmpty()) {
                download_list.emplace_back(uri);
            }
        }
    }

    if (!download_list.empty()) {
        emit startDownload(download_list);
    }

    return;
}

/**
 * @brief GkFileTransferDialog::on_pushButton_pause_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkFileTransferDialog::on_pushButton_pause_clicked()
{
    return;
}

/**
 * @brief GkFileTransferDialog::on_pushButton_open_dir_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkFileTransferDialog::on_pushButton_open_dir_clicked()
{
    return;
}

/**
 * @brief GkFileTransferDialog::on_pushButton_cancel_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkFileTransferDialog::on_pushButton_cancel_clicked()
{
    return;
}

/**
 * @brief GkFileTransferDialog::on_pushButton_abort_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkFileTransferDialog::on_pushButton_abort_clicked()
{
    return;
}

/**
 * @brief GkFileTransferDialog::on_progressBar_file_transfer_valueChanged
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param value
 */
void GkFileTransferDialog::on_progressBar_file_transfer_valueChanged(int value)
{
    return;
}

/**
 * @brief GkFileTransferDialog::recvDownloadHandle receives statistics about the file transfer itself, or rather, the
 * handle of the downloading file.
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param dh The download handle containing all the file transfer statistics.
 */
void GkFileTransferDialog::recvDownloadHandle(const std::shared_ptr<aria2::DownloadHandle> &dh)
{
    return;
}
