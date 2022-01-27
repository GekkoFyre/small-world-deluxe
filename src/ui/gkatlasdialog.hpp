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
#include "src/gk_logger.hpp"
#include <marble/MarbleWidget.h>
#include <marble/GeoDataCoordinates.h>
#include <QDialog>

namespace Ui {
class GkAtlasDialog;
}

class GkAtlasDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GkAtlasDialog(QPointer<GekkoFyre::GkEventLogger> eventLogger, QPointer<Marble::MarbleWidget> mapWidget,
                           QWidget *parent = nullptr);
    ~GkAtlasDialog();

private slots:
    void on_pushButton_accept_clicked();
    void on_pushButton_reset_clicked();
    void on_pushButton_close_clicked();

signals:
    void geoFocusPoint(const Marble::GeoDataCoordinates &coords);

private:
    Ui::GkAtlasDialog *ui;

    //
    // Events logger
    QPointer<GekkoFyre::GkEventLogger> gkEventLogger;

    //
    // Mapping and atlas APIs, etc.
    QPointer<Marble::MarbleWidget> m_mapWidget;

    //
    // Miscellaneous
    void initialize();

};
