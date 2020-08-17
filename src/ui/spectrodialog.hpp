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
 **   Small world is distributed in the hope that it will be useful,
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
 **   [ 1 ] - https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe
 **
 ****************************************************************************************************/

#pragma once

#include "src/defines.hpp"
#include "src/spectro_gui.hpp"
#include <QEvent>
#include <QObject>
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
    ~SpectroDialog() override;

private slots:
    void on_pushButton_apply_clicked();
    void on_pushButton_exit_clicked();
    void on_pushButton_export_graph_clicked();
    void on_pushButton_print_graph_clicked();
    void on_comboBox_graph_to_display_currentIndexChanged(int index);
    void on_comboBox_timing_currentIndexChanged(int index);
    void on_spinBox_fft_size_valueChanged(int arg1);

signals:
    void activateSpectroWaterfall(const bool &is_active);
    void changeGraphType(const GekkoFyre::Spectrograph::GkGraphType &graph_type);
    void changeFFTSize(const int &value);

private:
    Ui::SpectroDialog *ui;
    QPointer<GekkoFyre::SpectroGui> gkSpectroGui;

    int fft_size_prev_value;                        // Remembers the previous value, for if the user does not enter a number that's divisible by '256'!
    int fft_size_updated;
    bool fft_size_spinbox_sel;

    void prefillGraphTypes(const GekkoFyre::Spectrograph::GkGraphType &graph_type);
    void prefillGraphTiming(const GekkoFyre::Spectrograph::GkGraphTiming &graph_timing);
    bool eventFilter(QObject *obj, QEvent *event) override;
};

