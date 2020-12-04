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

#include "gkxmppmessagedialog.hpp"
#include "ui_gkxmppmessagedialog.h"
#include <QIcon>

GkXmppMessageDialog::GkXmppMessageDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::GkXmppMessageDialog)
{
    ui->setupUi(this);

    ui->label_callsign_2_icon->setPixmap(QPixmap(":/resources/contrib/images/vector/no-attrib/walkie-talkies.svg"));
    ui->toolButton_font->setIcon(QIcon(":/resources/contrib/images/vector/no-attrib/font.svg"));
    ui->toolButton_font_reset->setIcon(QIcon(":/resources/contrib/images/vector/no-attrib/eraser.svg"));
    ui->toolButton_insert->setIcon(QIcon(":/resources/contrib/images/vector/no-attrib/moustache-cream.svg"));
    ui->toolButton_attach_file->setIcon(QIcon(":/resources/contrib/images/vector/no-attrib/attached-file.svg"));
}

GkXmppMessageDialog::~GkXmppMessageDialog()
{
    delete ui;
}
