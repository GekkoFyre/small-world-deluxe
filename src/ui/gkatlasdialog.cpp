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
 **   Copyright (C) 2020 - 2021. GekkoFyre.
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

#include "src/ui/gkatlasdialog.hpp"
#include "ui_gkatlasdialog.h"

GkAtlasDialog::GkAtlasDialog(QPointer<GekkoFyre::GkEventLogger> eventLogger, QPointer<Marble::MarbleWidget> mapWidget,
                             QWidget *parent) : QDialog(parent), ui(new Ui::GkAtlasDialog)
{
    ui->setupUi(this);

    gkEventLogger = std::move(eventLogger);
    m_mapWidget = std::move(mapWidget);

    initialize();
}

GkAtlasDialog::~GkAtlasDialog()
{
    delete ui;
}

/**
 * @brief GkAtlasDialog::on_pushButton_accept_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkAtlasDialog::on_pushButton_accept_clicked()
{
    //
    // Accept current values and output them to the next destination!

    return;
}

/**
 * @brief GkAtlasDialog::on_pushButton_reset_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkAtlasDialog::on_pushButton_reset_clicked()
{
    //
    // Reset back to default values and layout!

    return;
}

/**
 * @brief GkAtlasDialog::on_pushButton_close_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkAtlasDialog::on_pushButton_close_clicked()
{
    //
    // Close QDialog()!
    this->close();

    return;
}

/**
 * @brief GkAtlasDialog::initialize As the title says; self-explanatory!
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void GkAtlasDialog::initialize()
{
    //
    // Resize the widget and display it to the end-user!
    ui->verticalLayout_3->addWidget(m_mapWidget);
    m_mapWidget->show();

    return;
}
