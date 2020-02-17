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

#pragma once

#include "src/defines.hpp"
#include "src/spectro_gui.hpp"
#include <QDialog>
#include <QString>
#include <QPointer>

namespace Ui {
class SpectroDialog;
}

class SpectroDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SpectroDialog(QPointer<GekkoFyre::SpectroGui> spectroGui, QWidget *parent = nullptr);
    ~SpectroDialog();

private slots:
    void on_pushButton_apply_clicked();
    void on_pushButton_reset_clicked();
    void on_pushButton_exit_clicked();
    void on_pushButton_contour_toggled(bool checked);
    void on_pushButton_activate_spectro_toggled(bool checked);
    void on_comboBox_colour_map_currentIndexChanged(int index);
    void on_comboBox_fft_size_currentIndexChanged(int index);
    void on_verticalSlider_control_alpha_valueChanged(int value);
    void on_horizontalSlider_control_colour_valueChanged(int value);

    void on_comboBox_ui_theme_activated(int index);

private:
    Ui::SpectroDialog *ui;
    QPointer<GekkoFyre::SpectroGui> gkSpectroGui;
};

