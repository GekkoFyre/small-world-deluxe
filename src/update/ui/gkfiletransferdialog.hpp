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

#include "src/update/gk_network.hpp"
#include <aria2/aria2.h>
#include <memory>
#include <string>
#include <vector>
#include <QString>
#include <QObject>
#include <QString>
#include <QWidget>
#include <QDialog>
#include <QPointer>

namespace Ui {
class GkFileTransferDialog;
}

class GkFileTransferDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GkFileTransferDialog(QWidget *parent = nullptr);
    ~GkFileTransferDialog();

public slots:
    void beginDownload(const std::vector<QString> &uris);

private slots:
    void on_pushButton_pause_clicked();
    void on_pushButton_open_dir_clicked();
    void on_pushButton_cancel_clicked();
    void on_pushButton_abort_clicked();
    void on_progressBar_file_transfer_valueChanged(int value);

    //
    // Aria2 related
    void recvDownloadHandle(const std::shared_ptr<aria2::DownloadHandle> &dh);

signals:
    void startDownload(const std::vector<QString> &uris);
    void pauseDownload(const std::vector<QString> &uris);
    void stopDownload(const std::vector<QString> &uris);

private:
    Ui::GkFileTransferDialog *ui;

    QPointer<GekkoFyre::GkNetwork> gkNetwork;
};
