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
 **   [ 1 ] - https://code.gekkofyre.io/amateur-radio/small-world-deluxe
 **
 ****************************************************************************************************/

#include "aboutdialog.hpp"
#include "ui_aboutdialog.h"
#include <QSize>
#include <QIcon>
#include <QPixmap>

using namespace GekkoFyre;
using namespace Database;
using namespace Settings;
using namespace Audio;
using namespace AmateurRadio;
using namespace Control;
using namespace Spectrograph;
using namespace System;
using namespace Events;
using namespace Logging;

AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);

    button_counter = 0;
    ui->pushButton_icon->setIcon(QIcon(":/resources/contrib/images/vector/purchased/2020-03/iconfinder_293_Frequency_News_Radio_5711690.svg"));
    ui->pushButton_icon->setIconSize(QSize(96, 96));
}

AboutDialog::~AboutDialog()
{
    delete ui;
}

void AboutDialog::on_buttonBox_close_rejected()
{
    this->close();
}

/**
 * @brief AboutDialog::on_pushButton_icon_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void AboutDialog::on_pushButton_icon_clicked()
{
    ++button_counter;
    switch (button_counter) {
        case GK_ABOUT_SCREEN_BTN_PIXMAP_RADIO_IDX:
            ui->pushButton_icon->setIcon(QIcon(":/resources/contrib/images/vector/purchased/2020-03/iconfinder_293_Frequency_News_Radio_5711690.svg"));
            ui->pushButton_icon->setIconSize(QSize(96, 96));
            break;
        case GK_ABOUT_SCREEN_BTN_PIXMAP_ARCTIC_COMM_ONE_IDX:
            ui->pushButton_icon->setIcon(QIcon(":/resources/contrib/images/raster/gekkofyre-networks/RaptorVonSqueaker/2019_RaptorVonSqueaker_artwork_commission_arctic_phobos_resized.png"));
            ui->pushButton_icon->setIconSize(QSize(96, 96));
            break;
        case GK_ABOUT_SCREEN_BTN_PIXMAP_ARCTIC_COMM_TWO_IDX:
            ui->pushButton_icon->setIcon(QIcon(":/resources/contrib/images/raster/gekkofyre-networks/Silberry/2020_silberry_artwork_commission_arctic_phobos_resized.png"));
            ui->pushButton_icon->setIconSize(QSize(96, 96));
            button_counter = 0;
            break;
        default:
            break;
    }

    return;
}
