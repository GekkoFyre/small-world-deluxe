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
 **   If you have downloaded the source code for "Dekoder for Morse" and are reading this,
 **   then thank you from the bottom of our hearts for making use of our hard work, sweat
 **   and tears in whatever you are implementing this into!
 **
 **   Copyright (C) 2020. GekkoFyre.
 **
 **   Dekoder for Morse is free software: you can redistribute it and/or modify
 **   it under the terms of the GNU General Public License as published by
 **   the Free Software Foundation, either version 3 of the License, or
 **   (at your option) any later version.
 **
 **   Dekoder is distributed in the hope that it will be useful,
 **   but WITHOUT ANY WARRANTY; without even the implied warranty of
 **   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **   GNU General Public License for more details.
 **
 **   You should have received a copy of the GNU General Public License
 **   along with Dekoder for Morse.  If not, see <http://www.gnu.org/licenses/>.
 **
 **
 **   The latest source code updates can be obtained from [ 1 ] below at your
 **   discretion. A web-browser or the 'git' application may be required.
 **
 **   [ 1 ] - https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe
 **
 ****************************************************************************************************/

#include "spectrodialog.hpp"
#include "ui_spectrodialog.h"
#include <QMessageBox>

using namespace GekkoFyre;

SpectroDialog::SpectroDialog(QPointer<GekkoFyre::SpectroGui> spectroGui, QWidget *parent) :
    QDialog(parent), ui(new Ui::SpectroDialog)
{
    ui->setupUi(this);
    this->adjustSize(); // Resize to contents

    gkSpectroGui = spectroGui;
}

SpectroDialog::~SpectroDialog()
{
    delete ui;
}

void SpectroDialog::on_pushButton_apply_clicked()
{
    return;
}

void SpectroDialog::on_pushButton_reset_clicked()
{
    return;
}

void SpectroDialog::on_pushButton_exit_clicked()
{
    this->close();
}

void SpectroDialog::on_pushButton_contour_toggled(bool checked)
{
    gkSpectroGui->showContour(checked);

    return;
}

void SpectroDialog::on_pushButton_activate_spectro_toggled(bool checked)
{
    return;
}

void SpectroDialog::on_comboBox_colour_map_currentIndexChanged(int index)
{
    switch (index) {
    case 0:
        gkSpectroGui->setColorMap(Spectrograph::GkColorMap::HueMap);
        return;
    case 1:
        gkSpectroGui->setColorMap(Spectrograph::GkColorMap::RGBMap);
        return;
    case 2:
        gkSpectroGui->setColorMap(Spectrograph::GkColorMap::AlphaMap);
        return;
    default:
        gkSpectroGui->setColorMap(Spectrograph::GkColorMap::HueMap);
        return;
    }

    return;
}

void SpectroDialog::on_comboBox_fft_size_currentIndexChanged(int index)
{
    return;
}

void SpectroDialog::on_verticalSlider_control_alpha_valueChanged(int value)
{
    gkSpectroGui->setAlpha(value);

    return;
}

void SpectroDialog::on_pushButton_export_graph_clicked()
{
    QMessageBox::information(this, tr("Information..."), tr("Apologies, but this function does not work yet."), QMessageBox::Ok);

    return;
}
